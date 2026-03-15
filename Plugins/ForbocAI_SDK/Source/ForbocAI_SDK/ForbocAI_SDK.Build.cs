using UnrealBuildTool;

public class ForbocAI_SDK : ModuleRules
{
	public ForbocAI_SDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HTTP", "Json", "JsonUtilities" });

		// --- NATIVE PARITY: llama.cpp & sqlite-vss ---
		string ThirdPartyPath = System.IO.Path.Combine(ModuleDirectory, "../../ThirdParty");
		string LlamaIncludePath = System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/include");
		string SqliteIncludePath = System.IO.Path.Combine(ThirdPartyPath, "sqlite-vss/include");
		string LlamaLibraryPath = null;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			LlamaLibraryPath = System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/lib/Win64/llama.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			LlamaLibraryPath = System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/lib/Mac/libllama.a");
		}

		bool bHasNativeLlama = System.IO.Directory.Exists(LlamaIncludePath)
			&& !string.IsNullOrEmpty(LlamaLibraryPath)
			&& System.IO.File.Exists(LlamaLibraryPath);
		bool bHasSqliteHeaders = System.IO.Directory.Exists(SqliteIncludePath);
		bool bHasSqlite3Header = bHasSqliteHeaders
			&& System.IO.File.Exists(System.IO.Path.Combine(SqliteIncludePath, "sqlite3.h"));

		// Auto-detect sqlite-vec amalgamation source files
		string SqliteAmalgamationPath = System.IO.Path.Combine(ThirdPartyPath, "sqlite-vss/src");
		bool bHasSqliteAmalgamation = System.IO.Directory.Exists(SqliteAmalgamationPath)
			&& System.IO.File.Exists(System.IO.Path.Combine(SqliteAmalgamationPath, "sqlite3.c"));

		if (bHasNativeLlama)
		{
			PublicIncludePaths.Add(LlamaIncludePath);
			PublicAdditionalLibraries.Add(LlamaLibraryPath);

			// Link ggml dependency libraries (llama.cpp depends on these)
			string LlamaLibDir = System.IO.Path.GetDirectoryName(LlamaLibraryPath);
			string[] GgmlLibs = { "libggml.a", "libggml-base.a", "libggml-cpu.a" };
			foreach (string GgmlLib in GgmlLibs)
			{
				string GgmlLibPath = System.IO.Path.Combine(LlamaLibDir, GgmlLib);
				if (System.IO.File.Exists(GgmlLibPath))
				{
					PublicAdditionalLibraries.Add(GgmlLibPath);
				}
			}

			// Metal backend (macOS GPU acceleration)
			if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string MetalLib = System.IO.Path.Combine(LlamaLibDir, "libggml-metal.a");
				if (System.IO.File.Exists(MetalLib))
				{
					PublicAdditionalLibraries.Add(MetalLib);
					PublicFrameworks.AddRange(new string[] { "Metal", "MetalKit", "Foundation" });
				}

				string BlasLib = System.IO.Path.Combine(LlamaLibDir, "libggml-blas.a");
				if (System.IO.File.Exists(BlasLib))
				{
					PublicAdditionalLibraries.Add(BlasLib);
					PublicFrameworks.Add("Accelerate");
				}
			}
		}

		if (bHasSqliteHeaders)
		{
			PublicIncludePaths.Add(SqliteIncludePath);
		}

		// Compile sqlite3 amalgamation + sqlite-vec into the plugin when source is present
		if (bHasSqliteAmalgamation)
		{
			PrivateIncludePaths.Add(SqliteAmalgamationPath);

			// The amalgamation compiles as C; UE treats .c files as C automatically
			string Sqlite3Source = System.IO.Path.Combine(SqliteAmalgamationPath, "sqlite3.c");
			string Vec0Source = System.IO.Path.Combine(SqliteAmalgamationPath, "vec0.c");

			// Suppress warnings in third-party C code
			bEnableExceptions = true;
		}

		PublicDefinitions.Add("WITH_FORBOC_NATIVE=" + (bHasNativeLlama ? "1" : "0"));
		// sqlite-vec auto-enabled when sqlite3.h header and amalgamation source are present
		bool bEnableSqliteVec = bHasSqlite3Header && bHasSqliteAmalgamation;
		PublicDefinitions.Add("WITH_FORBOC_SQLITE_VEC=" + (bEnableSqliteVec ? "1" : "0"));
	}
}
