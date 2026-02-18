using UnrealBuildTool;

public class ForbocAI_SDK : ModuleRules
{
	public ForbocAI_SDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HTTP", "Json", "JsonUtilities" });

		// --- NATIVE PARITY: llama.cpp & sqlite-vss ---
		string ThirdPartyPath = System.IO.Path.Combine(ModuleDirectory, "../../ThirdParty");
		
		// Llama.cpp
		PublicIncludePaths.Add(System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/include"));
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/lib/Win64/llama.lib"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(System.IO.Path.Combine(ThirdPartyPath, "llama.cpp/lib/Mac/libllama.a"));
		}

		// Sqlite-vss
		PublicIncludePaths.Add(System.IO.Path.Combine(ThirdPartyPath, "sqlite-vss/include"));
		// (Similar linking logic for sqlite-vss)

		PublicDefinitions.Add("WITH_FORBOC_NATIVE=1");

		// Unlock strict warnings for cleaner functional code
		// bEnableUndefinedIdentifierWarnings = false; 
	}
}
