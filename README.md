# 暮潮引灯人 · Lanterns at Low Tide

渡船迟迟未归，渡工的女儿 Mara 带着父亲留下的灯石走过秋色渡口。她要重新点亮两岸的航标，让渡船找到回家的路。七章的渡口日志伴随谜题推进，最后一盏灯点亮后，水面传来回应的铃声。

## 运行

本工程面向 Windows 和 Unreal Engine **5.5.4**。需要 MSVC v143 和 Windows SDK。克隆后打开 `RelayVault.uproject`，按提示编译，在 Unreal Editor 里点击 **Play**。3D 渡口会在游戏运行时生成。

如果双击时未找到 UE 5.5，或者需要手动编译，在 PowerShell 中修改第一行的引擎路径，从仓库根目录运行：

```powershell
$engine = 'D:\Program Files\Epic Games\UE_5.5'
& "$engine\Engine\Build\BatchFiles\Build.bat" RelayVaultEditor Win64 Development "-Project=$PWD\RelayVault.uproject" -WaitMutex
& "$engine\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\RelayVault.uproject"
```

## 规则

- **移动与推灯**　用 **WASD / 方向键**移动，一次推动一盏灯。把所有灯放进蓝色航标石座即可通关。
- **跃水**　灯石被推入水道后会沿推力方向越过连续的水格，落到越过水面后的第一格。如果落点在墙上或被占据，整次推灯失败。Mara 需要另找陆路。
- **潮镜**　站在任一面紫色潮镜上，或者与它上下左右相邻，按 **Q** 交换两面镜子上的内容。可以让一盏灯和空位交换，也可以让 Mara 与对面镜上的灯互换。
- **借灯开闸**　灯石压住金色石印时，所有闸门降下形成通路。移走最后一盏供灯后，闸门在所在格腾空时关闭。
- **流程**　**Z** 单步撤销，**R** 重开，**Enter** 开始或推进章节，**Esc / Backspace** 返回。跃水和潮镜交换各自算一步。

## 关卡递进

| 章节 | 设计目的 |
| --- | --- |
| 1 · The Empty Pier | 学会绕到灯石身后，把灯推入航标。 |
| 2 · Skipping Light | 从一个基本推灯动作中看到灯越过水道。 |
| 3 · The Silver Pair | 通过两个隔开的区域，理解潮镜交换的是格子上的内容。 |
| 4 · A Borrowed Flame | 学会留下一盏灯开闸，并在送完灯后取回它。 |
| 5 · Across Still Water | 让灯和旅人分头过河，在对岸重合。 |
| 6 · The Other Shore | 越过水道以后，还要借潮镜把灯送往下一岸。 |
| 7 · The Returning Light | 同时安排水道落点、潮镜的占位、闸门与回程。 |

《沉星之序》让不同区域分别建立角色、环境和规则，之后再让本来分开的机制交叉。本 Demo 取了其中的跃水与镜面换位思路，编成一趟渡口故事。前半段先分别教学，后半段让灯石的路线与 Mara 的路线分开，再在结尾汇合。[Arc Games 的区域与机制介绍](https://www.gamespress.com/NEW-EXPLORE-THE-REALM-TRAILER-HIGHLIGHTS-THE-MASSIVE-WORLD-WITHIN-ORDE)

## 关卡编辑器

- 在主界面按 **E** 或点击 **Open level editor**。选择 **Water** 或 **Mirror** 可以画水道或潮镜。潮镜必须恰好两面，闸门和石印配合使用。旅人和灯石只能放在干地上。
- 宽、高可设为 5–16 格。点击或拖动绘制，右键擦除，**Z** 撤销编辑。用 **Playtest** 进入 3D 试玩，返回会保留草稿。
- **Save slot / Load slot** 保存或加载到选定槽位。存档位于 `Saved/VaultLevels/slot-1.json` 至 `slot-3.json`，旧版存档可直接加载。

## 工程结构

`VaultBoard.cpp` 定义关卡和共用规则。3D 渡口由 `VaultCourtyard.cpp`、`VaultSurrounds.cpp` 和 `VaultGeometry.cpp` 生成，石材、符文和水面材质可在 `Content/Art/Materials` 编辑。UE 自动化测试 `RelayVault` 回放七章通关路线，验证机关撤销和编辑器存档往返。
