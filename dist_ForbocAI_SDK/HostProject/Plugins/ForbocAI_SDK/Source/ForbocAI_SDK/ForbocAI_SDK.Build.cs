using UnrealBuildTool;

public class ForbocAI_SDK : ModuleRules
{
	public ForbocAI_SDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HTTP", "Json", "JsonUtilities" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Unlock strict warnings for cleaner functional code
		// bEnableUndefinedIdentifierWarnings = false; 
	}
}
