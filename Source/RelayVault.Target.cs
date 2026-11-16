using UnrealBuildTool;

public class RelayVaultTarget : TargetRules
{
    public RelayVaultTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("RelayVault");
    }
}
