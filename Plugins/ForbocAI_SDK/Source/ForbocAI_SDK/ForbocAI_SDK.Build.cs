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

		if (bHasNativeLlama)
		{
			PublicIncludePaths.Add(LlamaIncludePath);
			PublicAdditionalLibraries.Add(LlamaLibraryPath);
		}

		if (bHasSqliteHeaders)
		{
			PublicIncludePaths.Add(SqliteIncludePath);
		}

		PublicDefinitions.Add("WITH_FORBOC_NATIVE=" + (bHasNativeLlama ? "1" : "0"));

		// Unlock strict warnings for cleaner functional code
		// bEnableUndefinedIdentifierWarnings = false; 
	}
}
