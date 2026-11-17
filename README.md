# Relay Vault · 中继金库

在秋色石庭里，将琥珀灯芯推到蓝色石座上。只有灯芯能激活金色继电板，旅人踏上去不会触发。只要任意一块继电板被压住，庭院里的闸门都会降下。移走最后一块板上的灯芯后，空闲的闸门格立即关闭，仍有物体占用的闸门格等腾空后再关闭。

## 运行

本工程面向 Windows 和 Unreal Engine **5.5.4**。需要安装 C++ 工具链（MSVC v143 和 Windows SDK）。克隆后双击 `RelayVault.uproject`，按提示编译，在 Unreal Editor 里点击 **Play**。庭院会在游戏开始时生成。

如果双击时未找到 UE 5.5，或者需要手动编译，在 PowerShell 中修改第一行的引擎路径，从仓库根目录运行：

```powershell
$engine = 'D:\Program Files\Epic Games\UE_5.5'
& "$engine\Engine\Build\BatchFiles\Build.bat" RelayVaultEditor Win64 Development "-Project=$PWD\RelayVault.uproject" -WaitMutex
& "$engine\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\RelayVault.uproject"
```

## 游玩

- 点击关卡行，或按 **Enter** 从第一关开始。**WASD / 方向键**移动，**Z** 撤销一步，**R** 重开。
- 所有灯芯都进入石座即可通关。按 **Enter** 进入下一关，**Esc / Backspace** 返回选关。
- 前两关练习推移和站位，第三关引入供电，后两关要先送货，再回收临时供电的灯芯。

## 关卡编辑器

- 在主界面选择 **Open level editor** 或按 **E**。在俯视网格上选择地块或物体，点击或拖动绘制。右键先擦除物体，再次右键恢复为空地。**Z** 撤销编辑。
- 宽、高可设为 5–16 格。放置一位旅人、至少一枚灯芯和等量石座。继电板和闸门配合使用。
- **Playtest** 在 3D 庭院中试玩当前草稿，返回编辑器会保留草稿。**Save slot** 保存到选定槽位，**Load slot** 重新载入。保存后的关卡也可从主界面直接进入。
- 自制关卡在 `Saved/VaultLevels/slot-1.json` 到 `slot-3.json`，可通过复制 JSON 分享。旧版本的自制关卡可直接加载。

## 工程结构

- `VaultBoard.cpp` 定义内置关卡和规则，供普通游玩和编辑器试玩共用。
- `VaultCourtyard.cpp`、`VaultSurrounds.cpp` 和 `VaultGeometry.cpp` 生成 3D 石庭、道具、旅人和周边环境，并根据棋盘状态更新物体。实际通行范围由网格决定。
- `Content/Art/Materials` 包含可在材质编辑器中调整的石材、符文和水面。场景使用工程自带的几何与材质，试玩时可通过 Eject 检查生成的网格组件。
