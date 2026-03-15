#include "CLI/CliHandlers.h"
#include "CLI/CliOperations.h"
#include "NativeEngine.h"
#include "RuntimeConfig.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace CLIOps {
namespace Handlers {

namespace {

/**
 * Version constants (single source of truth for ThirdParty setup)
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
static const TCHAR *SQLITE_VERSION = TEXT("3460100");
static const TCHAR *SQLITE_VEC_VERSION = TEXT("v0.1.6");
static const TCHAR *LLAMA_CPP_TAG = TEXT("b8191");

/**
 * Runs an external process synchronously.
 * User Story: As setup automation, I need process execution wrapped so build
 * and extraction commands can run with shared logging and timeout handling.
 */
int32 RunProcess(const FString &Executable, const FString &Args,
                 const FString &WorkingDir = TEXT(""),
                 float TimeoutSeconds = 300.0f) {
  FString StdOut;
  FString StdErr;
  int32 ReturnCode = -1;

  const FString EffectiveDir =
      WorkingDir.IsEmpty() ? FPaths::ProjectDir() : WorkingDir;

  UE_LOG(LogTemp, Display, TEXT("  [exec] %s %s"), *Executable, *Args);

  void *ReadPipe = nullptr;
  void *WritePipe = nullptr;
  FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

  FProcHandle Proc = FPlatformProcess::CreateProc(
      *Executable, *Args, false, true, true, nullptr, 0, *EffectiveDir,
      WritePipe, ReadPipe);

  if (!Proc.IsValid()) {
    UE_LOG(LogTemp, Warning, TEXT("  [FAIL] Could not launch %s"),
           *Executable);
    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    return -1;
  }

  /**
   * Wait with timeout
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  const double StartTime = FPlatformTime::Seconds();
  while (FPlatformProcess::IsProcRunning(Proc)) {
    StdOut += FPlatformProcess::ReadPipe(ReadPipe);
    if (FPlatformTime::Seconds() - StartTime > TimeoutSeconds) {
      FPlatformProcess::TerminateProc(Proc);
      UE_LOG(LogTemp, Warning, TEXT("  [FAIL] Process timed out after %.0fs"),
             TimeoutSeconds);
      FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
      return -1;
    }
    FPlatformProcess::Sleep(0.1f);
  }

  StdOut += FPlatformProcess::ReadPipe(ReadPipe);
  FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
  FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

  if (ReturnCode != 0 && !StdOut.IsEmpty()) {
    UE_LOG(LogTemp, Warning, TEXT("  %s"), *StdOut);
  }

  return ReturnCode;
}

/**
 * Extracts a zip file to a destination directory.
 * User Story: As dependency setup, I need zip extraction so downloaded archives
 * can be unpacked into the ThirdParty layout automatically.
 */
bool ExtractZip(const FString &ZipPath, const FString &DestDir) {
  IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();
  PF.CreateDirectoryTree(*DestDir);

#if PLATFORM_MAC || PLATFORM_LINUX
  return RunProcess(TEXT("/usr/bin/unzip"), FString::Printf(
      TEXT("-qo \"%s\" -d \"%s\""), *ZipPath, *DestDir)) == 0;
#elif PLATFORM_WINDOWS
  /**
   * PowerShell Expand-Archive
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  return RunProcess(TEXT("powershell.exe"), FString::Printf(
      TEXT("-NoProfile -Command \"Expand-Archive -Force -Path '%s' -DestinationPath '%s'\""),
      *ZipPath, *DestDir)) == 0;
#else
  UE_LOG(LogTemp, Warning, TEXT("  [FAIL] Zip extraction not supported on this platform"));
  return false;
#endif
}

/**
 * Extracts a tar.gz file to a destination directory.
 * User Story: As dependency setup, I need tarball extraction so downloaded
 * source archives can be unpacked into the expected build directories.
 */
bool ExtractTarGz(const FString &TarPath, const FString &DestDir) {
  IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();
  PF.CreateDirectoryTree(*DestDir);

#if PLATFORM_MAC || PLATFORM_LINUX
  return RunProcess(TEXT("/usr/bin/tar"), FString::Printf(
      TEXT("xzf \"%s\" -C \"%s\""), *TarPath, *DestDir)) == 0;
#elif PLATFORM_WINDOWS
  return RunProcess(TEXT("tar.exe"), FString::Printf(
      TEXT("xzf \"%s\" -C \"%s\""), *TarPath, *DestDir)) == 0;
#else
  return false;
#endif
}

/**
 * Finds the first subdirectory matching a prefix inside a directory.
 * User Story: As dependency setup, I need directory discovery so extracted
 * archives can be relocated without hard-coding versioned folder names.
 */
FString FindSubdirWithPrefix(const FString &Dir, const FString &Prefix) {
  TArray<FString> Dirs;
  IFileManager::Get().FindFiles(Dirs, *(Dir / TEXT("*")), false, true);
  for (const FString &D : Dirs) {
    if (D.StartsWith(Prefix)) {
      return Dir / D;
    }
  }
  return TEXT("");
}

/**
 * Reports ThirdParty dependency presence without modifying anything.
 * User Story: As setup diagnostics, I need a read-only verification pass so I
 * can inspect native dependency readiness before changing the workspace.
 */
Result VerifyThirdParty() {
  const FString PluginDir =
      FPaths::ProjectPluginsDir() / TEXT("ForbocAI_SDK");
  const FString ThirdPartyDir = PluginDir / TEXT("ThirdParty");

  const FString LlamaInclude = ThirdPartyDir / TEXT("llama.cpp/include");
  const FString SqliteInclude = ThirdPartyDir / TEXT("sqlite-vss/include");
  const FString SqliteSrc = ThirdPartyDir / TEXT("sqlite-vss/src");

  IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();

  const bool bLlamaHeaders = PF.DirectoryExists(*LlamaInclude) &&
                             PF.FileExists(*(LlamaInclude / TEXT("llama.h")));

#if PLATFORM_MAC
  const FString LlamaLib =
      ThirdPartyDir / TEXT("llama.cpp/lib/Mac/libllama.a");
#elif PLATFORM_WINDOWS
  const FString LlamaLib =
      ThirdPartyDir / TEXT("llama.cpp/lib/Win64/llama.lib");
#else
  const FString LlamaLib = TEXT("");
#endif
  const bool bLlamaLib =
      !LlamaLib.IsEmpty() && PF.FileExists(*LlamaLib);

#if PLATFORM_MAC
  const FString LlamaLibDir = ThirdPartyDir / TEXT("llama.cpp/lib/Mac");
  const bool bGgmlCore =
      PF.FileExists(*(LlamaLibDir / TEXT("libggml.a"))) &&
      PF.FileExists(*(LlamaLibDir / TEXT("libggml-base.a"))) &&
      PF.FileExists(*(LlamaLibDir / TEXT("libggml-cpu.a")));
  const bool bGgmlMetal =
      PF.FileExists(*(LlamaLibDir / TEXT("libggml-metal.a")));
  const bool bGgmlBlas =
      PF.FileExists(*(LlamaLibDir / TEXT("libggml-blas.a")));
#elif PLATFORM_WINDOWS
  const FString LlamaLibDir = ThirdPartyDir / TEXT("llama.cpp/lib/Win64");
  const bool bGgmlCore =
      PF.FileExists(*(LlamaLibDir / TEXT("ggml.lib"))) &&
      PF.FileExists(*(LlamaLibDir / TEXT("ggml-base.lib"))) &&
      PF.FileExists(*(LlamaLibDir / TEXT("ggml-cpu.lib")));
  const bool bGgmlMetal = false;
  const bool bGgmlBlas = false;
#else
  const FString LlamaLibDir = TEXT("");
  const bool bGgmlCore = false;
  const bool bGgmlMetal = false;
  const bool bGgmlBlas = false;
#endif

  const bool bSqliteHeaders = PF.DirectoryExists(*SqliteInclude) &&
                              PF.FileExists(*(SqliteInclude / TEXT("sqlite3.h")));
  const bool bSqliteAmalgamation =
      PF.DirectoryExists(*SqliteSrc) &&
      PF.FileExists(*(SqliteSrc / TEXT("sqlite3.c")));
  const bool bVec0 = PF.FileExists(*(SqliteSrc / TEXT("vec0.c")));

  const FString LocalInfra = FPaths::ProjectDir() / TEXT("local_infrastructure");
  const FString ModelsDir = LocalInfra / TEXT("models");
  TArray<FString> ModelFiles;
  PF.FindFiles(ModelFiles, *ModelsDir, TEXT(".gguf"));
  const int32 ModelCount = ModelFiles.Num();

  const FString VectorsDir = LocalInfra / TEXT("vectors");
  const bool bVectorDb = PF.DirectoryExists(*VectorsDir);

  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("=== ForbocAI ThirdParty Dependency Check ==="));
  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("  [%s] llama.cpp headers     (%s)"),
         bLlamaHeaders ? TEXT("OK") : TEXT("--"), *LlamaInclude);
  UE_LOG(LogTemp, Display, TEXT("  [%s] llama.cpp library     (%s)"),
         bLlamaLib ? TEXT("OK") : TEXT("--"), *LlamaLib);
  UE_LOG(LogTemp, Display, TEXT("  [%s] ggml core libs        (ggml, ggml-base, ggml-cpu)"),
         bGgmlCore ? TEXT("OK") : TEXT("--"));
#if PLATFORM_MAC
  UE_LOG(LogTemp, Display, TEXT("  [%s] ggml-metal            (%s)"),
         bGgmlMetal ? TEXT("OK") : TEXT("--"),
         *(LlamaLibDir / TEXT("libggml-metal.a")));
  UE_LOG(LogTemp, Display, TEXT("  [%s] ggml-blas             (%s)"),
         bGgmlBlas ? TEXT("OK") : TEXT("--"),
         *(LlamaLibDir / TEXT("libggml-blas.a")));
#endif
  UE_LOG(LogTemp, Display, TEXT("  [%s] sqlite3 headers       (%s)"),
         bSqliteHeaders ? TEXT("OK") : TEXT("--"), *SqliteInclude);
  UE_LOG(LogTemp, Display, TEXT("  [%s] sqlite3 amalgamation  (%s)"),
         bSqliteAmalgamation ? TEXT("OK") : TEXT("--"), *SqliteSrc);
  UE_LOG(LogTemp, Display, TEXT("  [%s] sqlite-vec (vec0.c)   (%s)"),
         bVec0 ? TEXT("OK") : TEXT("--"),
         *(SqliteSrc / TEXT("vec0.c")));
  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("  Compile defines:"));
  UE_LOG(LogTemp, Display, TEXT("    WITH_FORBOC_NATIVE     = %d"),
         WITH_FORBOC_NATIVE);
  UE_LOG(LogTemp, Display, TEXT("    WITH_FORBOC_SQLITE_VEC = %d"),
         WITH_FORBOC_SQLITE_VEC);
  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("  Runtime assets:"));
  UE_LOG(LogTemp, Display, TEXT("  [%s] GGUF models           (%d file(s) in %s)"),
         ModelCount > 0 ? TEXT("OK") : TEXT("--"), ModelCount, *ModelsDir);
  UE_LOG(LogTemp, Display, TEXT("  [%s] Vector DB             (%s)"),
         bVectorDb ? TEXT("OK") : TEXT("--"), *VectorsDir);
  UE_LOG(LogTemp, Display, TEXT(""));

  if (!bLlamaHeaders || !bLlamaLib) {
    UE_LOG(LogTemp, Display, TEXT(
        "  To set up: setup_deps --llama-only, then setup_build_llama"));
  }
  if (!bSqliteHeaders || !bSqliteAmalgamation || !bVec0) {
    UE_LOG(LogTemp, Display, TEXT(
        "  To set up: setup_deps --sqlite-only"));
  }
  if (ModelCount == 0) {
    UE_LOG(LogTemp, Display, TEXT(
        "  Models download automatically on first cortex_init."));
  }

  const bool bAllBuild = bLlamaHeaders && bLlamaLib && bGgmlCore &&
                         bSqliteHeaders && bSqliteAmalgamation && bVec0;
  return bAllBuild ? Result::Success("All build-time dependencies present")
                   : Result::Failure("Some build-time dependencies missing");
}

/**
 * Installs or verifies the ThirdParty dependency bundle.
 * User Story: As setup flows, I need one dependency installer so llama.cpp and
 * sqlite-vss assets can be downloaded and arranged predictably.
 */
Result SetupThirdPartyDeps(rtk::EnhancedStore<FStoreState> &Store,
                           const TArray<FString> &Args) {
  const FString PluginDir =
      FPaths::ProjectPluginsDir() / TEXT("ForbocAI_SDK");
  const FString ThirdPartyDir = PluginDir / TEXT("ThirdParty");
  const FString TmpDir =
      FPaths::ProjectDir() / TEXT("local_infrastructure/tmp");

  IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();

  bool bDoSqlite = true;
  bool bDoLlama = true;
  for (const FString &Arg : Args) {
    if (Arg == TEXT("--sqlite-only")) {
      bDoLlama = false;
    } else if (Arg == TEXT("--llama-only")) {
      bDoSqlite = false;
    }
  }

  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("=== ForbocAI ThirdParty Setup ==="));

  const FString LlamaIncDir = ThirdPartyDir / TEXT("llama.cpp/include");
  const FString LlamaLibMac = ThirdPartyDir / TEXT("llama.cpp/lib/Mac");
  const FString LlamaLibWin = ThirdPartyDir / TEXT("llama.cpp/lib/Win64");
  const FString SqliteIncDir = ThirdPartyDir / TEXT("sqlite-vss/include");
  const FString SqliteSrcDir = ThirdPartyDir / TEXT("sqlite-vss/src");

  PF.CreateDirectoryTree(*LlamaIncDir);
  PF.CreateDirectoryTree(*LlamaLibMac);
  PF.CreateDirectoryTree(*LlamaLibWin);
  PF.CreateDirectoryTree(*SqliteIncDir);
  PF.CreateDirectoryTree(*SqliteSrcDir);
  PF.CreateDirectoryTree(*TmpDir);

  int32 DownloadCount = 0;
  int32 FailCount = 0;

  auto DownloadOne = [&Store, &DownloadCount,
                      &FailCount](const FString &Url, const FString &Dest) -> bool {
    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*Dest)) {
      UE_LOG(LogTemp, Display, TEXT("  [skip] %s (exists)"),
             *FPaths::GetCleanFilename(Dest));
      return true;
    }
    UE_LOG(LogTemp, Display, TEXT("  [download] %s"),
           *FPaths::GetCleanFilename(Dest));
    try {
      Ops::WaitForResult(Native::File::DownloadBinary(Url, Dest), 120.0);
      ++DownloadCount;
      return true;
    } catch (const std::exception &E) {
      UE_LOG(LogTemp, Warning, TEXT("  [FAIL] %s: %s"),
             *FPaths::GetCleanFilename(Dest),
             UTF8_TO_TCHAR(E.what()));
      ++FailCount;
      return false;
    }
  };

  /**
   * sqlite3 amalgamation + sqlite-vec (replaces vendor_sqlite in script)
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (bDoSqlite) {
    UE_LOG(LogTemp, Display, TEXT(""));
    UE_LOG(LogTemp, Display, TEXT("  --- sqlite3 + sqlite-vec ---"));

    const FString Sqlite3hDest = SqliteIncDir / TEXT("sqlite3.h");
    const FString Sqlite3extDest = SqliteIncDir / TEXT("sqlite3ext.h");
    const FString Sqlite3cDest = SqliteSrcDir / TEXT("sqlite3.c");

    /**
     * Download + extract sqlite3 amalgamation if any file missing
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    if (!PF.FileExists(*Sqlite3hDest) || !PF.FileExists(*Sqlite3cDest)) {
      const FString SqliteZipUrl = FString::Printf(
          TEXT("https://www.sqlite.org/2024/sqlite-amalgamation-%s.zip"),
          SQLITE_VERSION);
      const FString SqliteZip = TmpDir / TEXT("sqlite-amalgamation.zip");
      const FString SqliteExtractDir = TmpDir / TEXT("sqlite-extract");

      if (DownloadOne(SqliteZipUrl, SqliteZip)) {
        UE_LOG(LogTemp, Display, TEXT("  [extract] sqlite-amalgamation.zip"));
        if (ExtractZip(SqliteZip, SqliteExtractDir)) {
          const FString InnerDir = FindSubdirWithPrefix(
              SqliteExtractDir, TEXT("sqlite-amalgamation-"));

          if (!InnerDir.IsEmpty()) {
            IFileManager &FM = IFileManager::Get();
            FM.Copy(*Sqlite3hDest, *(InnerDir / TEXT("sqlite3.h")));
            FM.Copy(*Sqlite3extDest, *(InnerDir / TEXT("sqlite3ext.h")));
            FM.Copy(*Sqlite3cDest, *(InnerDir / TEXT("sqlite3.c")));
            UE_LOG(LogTemp, Display, TEXT("  [OK] sqlite3 amalgamation installed"));
          } else {
            UE_LOG(LogTemp, Warning,
                   TEXT("  [FAIL] Could not find sqlite-amalgamation-* in archive"));
            ++FailCount;
          }
        } else {
          ++FailCount;
        }
      }
    } else {
      UE_LOG(LogTemp, Display, TEXT("  [skip] sqlite3 amalgamation (exists)"));
    }

    /**
     * Download + extract sqlite-vec
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    const FString Vec0Dest = SqliteSrcDir / TEXT("vec0.c");
    if (!PF.FileExists(*Vec0Dest)) {
      const FString VecZipUrl = FString::Printf(
          TEXT("https://github.com/asg017/sqlite-vec/releases/download/%s/"
               "sqlite-vec-%s-amalgamation.zip"),
          SQLITE_VEC_VERSION,
          *FString(SQLITE_VEC_VERSION).Replace(TEXT("v"), TEXT("")));
      const FString VecZip = TmpDir / TEXT("sqlite-vec-amalgamation.zip");
      const FString VecExtractDir = TmpDir / TEXT("sqlite-vec-extract");

      if (DownloadOne(VecZipUrl, VecZip)) {
        UE_LOG(LogTemp, Display, TEXT("  [extract] sqlite-vec-amalgamation.zip"));
        if (ExtractZip(VecZip, VecExtractDir)) {
          /**
           * sqlite-vec amalgamation zip contains sqlite-vec.c at root
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          IFileManager &FM = IFileManager::Get();
          const FString VecSrc1 = VecExtractDir / TEXT("sqlite-vec.c");
          const FString VecSrc2 = VecExtractDir / TEXT("vec0.c");
          /**
           * Check for nested dir too
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          const FString VecSubDir = FindSubdirWithPrefix(
              VecExtractDir, TEXT("sqlite-vec-"));
          const FString VecSrc3 =
              VecSubDir.IsEmpty() ? TEXT("")
                                 : (VecSubDir / TEXT("sqlite-vec.c"));

          if (PF.FileExists(*VecSrc1)) {
            FM.Copy(*Vec0Dest, *VecSrc1);
          } else if (PF.FileExists(*VecSrc2)) {
            FM.Copy(*Vec0Dest, *VecSrc2);
          } else if (!VecSrc3.IsEmpty() && PF.FileExists(*VecSrc3)) {
            FM.Copy(*Vec0Dest, *VecSrc3);
          } else {
            /**
             * Try src/ subdirectory
             * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
             */
            const FString VecSrc4 =
                VecSubDir.IsEmpty()
                    ? TEXT("")
                    : (VecSubDir / TEXT("src/sqlite-vec.c"));
            if (!VecSrc4.IsEmpty() && PF.FileExists(*VecSrc4)) {
              FM.Copy(*Vec0Dest, *VecSrc4);
            } else {
              UE_LOG(LogTemp, Warning,
                     TEXT("  [FAIL] Could not find sqlite-vec.c or vec0.c in archive"));
              ++FailCount;
            }
          }

          if (PF.FileExists(*Vec0Dest)) {
            UE_LOG(LogTemp, Display, TEXT("  [OK] sqlite-vec (vec0.c) installed"));
          }

          /**
           * Copy header if present
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          const FString VecHdrSrc =
              VecSubDir.IsEmpty() ? (VecExtractDir / TEXT("sqlite-vec.h"))
                                 : (VecSubDir / TEXT("sqlite-vec.h"));
          if (PF.FileExists(*VecHdrSrc)) {
            FM.Copy(*(SqliteIncDir / TEXT("sqlite-vec.h")), *VecHdrSrc);
          }
        } else {
          ++FailCount;
        }
      }
    } else {
      UE_LOG(LogTemp, Display, TEXT("  [skip] sqlite-vec vec0.c (exists)"));
    }
  }

  /**
   * llama.cpp headers (replaces vendor_llama_headers in script)
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  if (bDoLlama) {
    UE_LOG(LogTemp, Display, TEXT(""));
    UE_LOG(LogTemp, Display, TEXT("  --- llama.cpp headers (%s) ---"), LLAMA_CPP_TAG);

    const FString LlamaBase = FString::Printf(
        TEXT("https://raw.githubusercontent.com/ggml-org/llama.cpp/%s"),
        LLAMA_CPP_TAG);

    DownloadOne(LlamaBase / TEXT("include/llama.h"),
                LlamaIncDir / TEXT("llama.h"));
    DownloadOne(LlamaBase / TEXT("ggml/include/ggml.h"),
                LlamaIncDir / TEXT("ggml.h"));
    DownloadOne(LlamaBase / TEXT("ggml/include/ggml-alloc.h"),
                LlamaIncDir / TEXT("ggml-alloc.h"));
    DownloadOne(LlamaBase / TEXT("ggml/include/ggml-backend.h"),
                LlamaIncDir / TEXT("ggml-backend.h"));

    UE_LOG(LogTemp, Display, TEXT(""));
    UE_LOG(LogTemp, Display, TEXT(
        "  To build libllama, run: setup_build_llama"));
  }

  /**
   * Clean up temp dir
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  IFileManager::Get().DeleteDirectory(*TmpDir, true, true);

  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("  Downloaded: %d | Failed: %d"),
         DownloadCount, FailCount);
  UE_LOG(LogTemp, Display, TEXT(
      "  Rebuild the UE project to pick up WITH_FORBOC_NATIVE / WITH_FORBOC_SQLITE_VEC."));
  UE_LOG(LogTemp, Display, TEXT(""));

  return FailCount == 0
             ? Result::Success("ThirdParty setup completed")
             : Result::Failure("Some downloads failed — see logs above");
}

/**
 * Builds the native llama.cpp library for the current platform.
 * User Story: As native setup, I need a build helper so the local inference
 * library can be compiled after sources are downloaded.
 */
Result BuildLlama(const TArray<FString> &Args) {
  const FString PluginDir =
      FPaths::ProjectPluginsDir() / TEXT("ForbocAI_SDK");
  const FString ThirdPartyDir = PluginDir / TEXT("ThirdParty");
  const FString TmpDir =
      FPaths::ProjectDir() / TEXT("local_infrastructure/tmp/llama-build");

  FString Tag = LLAMA_CPP_TAG;
  for (const FString &Arg : Args) {
    if (Arg.StartsWith(TEXT("--tag="))) {
      Tag = Arg.Mid(6);
    }
  }

  IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();

  /**
   * Check if lib already exists
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
#if PLATFORM_MAC
  const FString LibDest = ThirdPartyDir / TEXT("llama.cpp/lib/Mac/libllama.a");
  const FString LibName = TEXT("libllama.a");
#elif PLATFORM_WINDOWS
  const FString LibDest = ThirdPartyDir / TEXT("llama.cpp/lib/Win64/llama.lib");
  const FString LibName = TEXT("llama.lib");
#else
  return Result::Failure("build_llama not supported on this platform");
#endif

  if (PF.FileExists(*LibDest)) {
    UE_LOG(LogTemp, Display, TEXT("  [skip] %s already exists at %s"),
           *LibName, *LibDest);
    return Result::Success("llama library already present");
  }

  UE_LOG(LogTemp, Display, TEXT(""));
  UE_LOG(LogTemp, Display, TEXT("=== Building llama.cpp (%s) ==="), *Tag);
  UE_LOG(LogTemp, Display, TEXT("  This may take several minutes..."));
  UE_LOG(LogTemp, Display, TEXT(""));

  /**
   * Clean and create build dir
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  IFileManager::Get().DeleteDirectory(*TmpDir, true, true);
  PF.CreateDirectoryTree(*TmpDir);

  /**
   * Step 1: Clone llama.cpp
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  UE_LOG(LogTemp, Display, TEXT("  Step 1/3: Cloning llama.cpp..."));
  const FString CloneDir = TmpDir / TEXT("llama.cpp");

#if PLATFORM_MAC || PLATFORM_LINUX
  const FString GitExe = TEXT("/usr/bin/git");
#elif PLATFORM_WINDOWS
  const FString GitExe = TEXT("git.exe");
#endif

  int32 Rc = RunProcess(GitExe, FString::Printf(
      TEXT("clone --depth 1 --branch %s https://github.com/ggml-org/llama.cpp \"%s\""),
      *Tag, *CloneDir), TEXT(""), 120.0f);
  if (Rc != 0) {
    return Result::Failure("git clone failed");
  }

  /**
   * Step 2: CMake configure
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  UE_LOG(LogTemp, Display, TEXT("  Step 2/3: Configuring with CMake..."));
  const FString BuildDir = CloneDir / TEXT("build");

#if PLATFORM_MAC || PLATFORM_LINUX
  /**
   * Try Homebrew paths first (Apple Silicon then Intel), fallback to PATH
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  const FString CmakeExe =
      PF.FileExists(TEXT("/opt/homebrew/bin/cmake"))
          ? FString(TEXT("/opt/homebrew/bin/cmake"))
          : (PF.FileExists(TEXT("/usr/local/bin/cmake"))
                 ? FString(TEXT("/usr/local/bin/cmake"))
                 : FString(TEXT("cmake")));
  const FString CmakeExeAlt = TEXT("cmake");
#elif PLATFORM_WINDOWS
  const FString CmakeExe = TEXT("cmake.exe");
  const FString CmakeExeAlt = CmakeExe;
#endif

  FString CmakeConfigArgs = FString::Printf(
      TEXT("-B \"%s\" -DBUILD_SHARED_LIBS=OFF"), *BuildDir);

#if PLATFORM_MAC
  CmakeConfigArgs += TEXT(" -DGGML_METAL=ON");
#endif

  Rc = RunProcess(CmakeExe, CmakeConfigArgs, CloneDir, 120.0f);
  if (Rc != 0) {
    /**
     * Retry with alternative cmake path
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    Rc = RunProcess(CmakeExeAlt, CmakeConfigArgs, CloneDir, 120.0f);
    if (Rc != 0) {
      return Result::Failure("cmake configure failed — is cmake installed?");
    }
  }

  /**
   * Step 3: Build
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  UE_LOG(LogTemp, Display, TEXT("  Step 3/3: Building llama target..."));
  const FString CmakeBuildArgs = FString::Printf(
      TEXT("--build \"%s\" --target llama -j"), *BuildDir);

  Rc = RunProcess(CmakeExe, CmakeBuildArgs, CloneDir, 600.0f);
  if (Rc != 0) {
    Rc = RunProcess(CmakeExeAlt, CmakeBuildArgs, CloneDir, 600.0f);
    if (Rc != 0) {
      return Result::Failure("cmake build failed");
    }
  }

  /**
   * Copy llama + ggml libraries to ThirdParty
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
#if PLATFORM_MAC
  const FString BuiltLib = BuildDir / TEXT("src/libllama.a");
#elif PLATFORM_WINDOWS
  const FString BuiltLib = BuildDir / TEXT("src/Release/llama.lib");
#endif

  PF.CreateDirectoryTree(*FPaths::GetPath(LibDest));

  if (PF.FileExists(*BuiltLib)) {
    IFileManager::Get().Copy(*LibDest, *BuiltLib);
    UE_LOG(LogTemp, Display, TEXT(""));
    UE_LOG(LogTemp, Display, TEXT("  [OK] %s copied to %s"), *LibName, *LibDest);
  } else {
    UE_LOG(LogTemp, Warning, TEXT("  [FAIL] Built library not found at %s"),
           *BuiltLib);
    return Result::Failure("Build succeeded but library not found");
  }

  /**
   * Copy ggml dependency libraries (Build.cs links these alongside libllama)
   * User Story: As build setup, I need ggml libs copied so UE linking succeeds.
   */
  {
    const FString LibDestDir = FPaths::GetPath(LibDest);
    IFileManager &FM = IFileManager::Get();
    int32 GgmlCopied = 0;

#if PLATFORM_MAC
    struct GgmlEntry {
      const TCHAR *BuildSubPath;
      const TCHAR *DestName;
    };
    const GgmlEntry GgmlLibs[] = {
        {TEXT("ggml/src/libggml.a"), TEXT("libggml.a")},
        {TEXT("ggml/src/libggml-base.a"), TEXT("libggml-base.a")},
        {TEXT("ggml/src/ggml-cpu/libggml-cpu.a"), TEXT("libggml-cpu.a")},
        {TEXT("ggml/src/ggml-metal/libggml-metal.a"), TEXT("libggml-metal.a")},
        {TEXT("ggml/src/ggml-blas/libggml-blas.a"), TEXT("libggml-blas.a")},
    };
#elif PLATFORM_WINDOWS
    struct GgmlEntry {
      const TCHAR *BuildSubPath;
      const TCHAR *DestName;
    };
    const GgmlEntry GgmlLibs[] = {
        {TEXT("ggml/src/Release/ggml.lib"), TEXT("ggml.lib")},
        {TEXT("ggml/src/Release/ggml-base.lib"), TEXT("ggml-base.lib")},
        {TEXT("ggml/src/ggml-cpu/Release/ggml-cpu.lib"), TEXT("ggml-cpu.lib")},
    };
#endif

    for (const GgmlEntry &Entry : GgmlLibs) {
      const FString Src = BuildDir / Entry.BuildSubPath;
      const FString Dst = LibDestDir / Entry.DestName;
      if (PF.FileExists(*Src)) {
        FM.Copy(*Dst, *Src);
        UE_LOG(LogTemp, Display, TEXT("  [OK] %s"), Entry.DestName);
        ++GgmlCopied;
      } else {
        UE_LOG(LogTemp, Warning, TEXT("  [--] %s not found at %s"),
               Entry.DestName, *Src);
      }
    }

    UE_LOG(LogTemp, Display, TEXT("  Copied %d ggml libraries"), GgmlCopied);
  }

  /**
   * Clean up build dir
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  IFileManager::Get().DeleteDirectory(*TmpDir, true, true);

  UE_LOG(LogTemp, Display, TEXT(
      "  Rebuild the UE project to enable WITH_FORBOC_NATIVE=1."));
  UE_LOG(LogTemp, Display, TEXT(""));

  return Result::Success("llama.cpp built and installed");
}

} // anonymous namespace

/**
 * Routes setup-related CLI commands to the appropriate setup helper.
 * User Story: As CLI users, I need setup commands dispatched through one
 * handler so verification, install, and build flows share parsing logic.
 */
HandlerResult HandleSetup(rtk::EnhancedStore<FStoreState> &Store,
                          const FString &CommandKey,
                          const TArray<FString> &Args) {
  using func::just;
  using func::nothing;

  if (CommandKey == TEXT("setup") || CommandKey == TEXT("setup_deps")) {
    return just(SetupThirdPartyDeps(Store, Args));
  }

  if (CommandKey == TEXT("setup_check") || CommandKey == TEXT("setup_verify")) {
    (void)Store;
    return just(VerifyThirdParty());
  }

  if (CommandKey == TEXT("setup_build_llama")) {
    (void)Store;
    return just(BuildLlama(Args));
  }

  return nothing<Result>();
}

} // namespace Handlers
} // namespace CLIOps
