using UnrealBuildTool;

public class RelayVault : ModuleRules
{
    public RelayVault(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "ProceduralMeshComponent" });
    }
}
