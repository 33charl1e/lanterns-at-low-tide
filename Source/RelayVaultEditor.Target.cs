using UnrealBuildTool;

public class RelayVaultEditorTarget : TargetRules
{
    public RelayVaultEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
        ExtraModuleNames.Add("RelayVault");
    }
}
