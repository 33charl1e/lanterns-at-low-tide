# Relay Vault · 中继金库

仓储机器人要把所有电池送进充电座。压住条纹继电板的必须是电池，机器人走上去不会通电。只要有任意一块继电板被电池压住，所有闸门都会打开。撤掉最后一块板上的电池会断电；闸门格被占用时会保持通行，直到该格腾空。

## 运行

本工程面向 Windows 和 Unreal Engine **5.5.4**。需要安装 C++ 编译工具链（MSVC v143 和 Windows SDK）。克隆后双击 `RelayVault.uproject`，按提示编译。打开编辑器后点击 **Play**，即可进入 **Battery dispatch**。PIE 获得焦点后可用键盘控制。

如果双击时未找到 UE 5.5，或者需要手动编译，在 PowerShell 中修改第一行的引擎路径，并从仓库根目录运行下列命令。

```powershell
$engine = 'D:\Program Files\Epic Games\UE_5.5'
& "$engine\Engine\Build\BatchFiles\Build.bat" RelayVaultEditor Win64 Development "-Project=$PWD\RelayVault.uproject" -WaitMutex
& "$engine\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\RelayVault.uproject"
```

## 游玩

- 点击关卡行或按 **Enter** 从第一关开始。**WASD / 方向键**移动，**Z** 撤销一步，**R** 重开。
- 将所有电池推到蓝色充电座上即可通关。点击 **Next sector** 或按 **Enter** 进入下一关，**Esc / Backspace** 返回选关。
- 前两关练习推移和站位，第三关开始引入供电，后两关要求先送货、再回收提供临时电力的电池。

## 关卡编辑器

- 在主页点击 **Open level editor** 或按 **E**。选择地块或实体后点击或拖动绘制。右键先擦除实体，再次右键恢复为空地。**Z** 撤销编辑。
- 可设置 5–16 格的宽度和高度。必须放置一个机器人、至少一个电池、同样数量的充电座。继电板和闸门要同时出现。可用 **Playtest** 验证关卡是否可解。
- **Playtest** 从当前草稿开始，返回时保留草稿。用 **Save slot** 保存到当前选定的槽位；可用 **Load slot** 重新编辑，或回到主页直接游玩。
- 自制关保存在 `Saved/VaultLevels/slot-1.json` 到 `slot-3.json`。可通过复制 JSON 交换自制关卡。内置关卡定义在 `Source/RelayVault/Private/VaultBoard.cpp`。

棋盘规则、关卡数据与绘制分开组织。游戏和编辑器试玩使用同一个棋盘规则，Undo 恢复玩家、电池和计数，通电状态由恢复后的棋盘重新计算。界面和棋盘使用 UE Canvas 绘制，工程使用引擎自带空地图和字体。
