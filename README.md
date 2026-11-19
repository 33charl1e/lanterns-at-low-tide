# 暮潮引灯人

渡船迟迟未归，渡工的女儿 Mara 带着父亲留下的灯石走过秋色渡口。她要重新点亮两岸的航标，让渡船找到回家的路。七章的渡口日志伴随谜题推进，最后一盏灯点亮后，水面传来回应的铃声。

## 运行

本工程面向 Windows 和 Unreal Engine **5.5.4**。需要 MSVC v143 和 Windows SDK。克隆后打开 `RelayVault.uproject`，按提示编译，在 Unreal Editor 里点击 **Play**。首页的海岛和渡船在运行时生成。点击 **开始游玩** 打开渡口手记，点击 **退出游戏** 结束游玩。

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
- **流程**　**Z** 单步撤销，**R** 重开，**Enter** 从首页进入选关、从选关进入第一章，或推进章节，**Esc / Backspace** 返回。跃水和潮镜交换各自算一步。

## 关卡递进

| 章节 | 设计目的 |
| --- | --- |
| 1 · 空码头 | 学会绕到灯石身后，把灯推入航标。 |
| 2 · 跃水灯 | 从一个基本推灯动作中看到灯越过水道。 |
| 3 · 一对潮镜 | 通过两个隔开的区域，理解潮镜交换的是格子上的内容。 |
| 4 · 借一盏灯 | 学会留下一盏灯开闸，并在送完灯后取回它。 |
| 5 · 静水两岸 | 先把灯石跃送过河，借另一盏灯开闸，再绕到落点旁将灯推进航标。 |
| 6 · 镜水彼岸 | 让两盏灯分别过水，再用同一对潮镜送往两个航标。 |
| 7 · 归航之灯 | 安排三盏灯的运送顺序，让两盏灯过水和潮镜，留第三盏开闸直至回程。 |

《沉星之序》让不同区域分别建立角色、环境和规则，之后再让本来分开的机制交叉。本 Demo 取了其中的跃水与镜面换位思路，编成一趟渡口故事。前半段先分别教学，后半段让灯石的路线与 Mara 的路线分开，再在结尾汇合。[Arc Games 的区域与机制介绍](https://www.gamespress.com/NEW-EXPLORE-THE-REALM-TRAILER-HIGHLIGHTS-THE-MASSIVE-WORLD-WITHIN-ORDE)

## 关卡编辑器

- 在选关界面按 **E** 或点击 **编辑关卡**。选择 **水道** 或 **潮镜** 可以画水道或潮镜。潮镜必须恰好两面，闸门和石印配合使用。旅人和灯石只能放在干地上。
- 宽、高可设为 5–16 格。点击或拖动绘制，右键擦除，**Z** 撤销编辑。用 **立体预览** 查看当前草稿，尚未布置完整的地图也可以预览。通过 **返回草图** 继续绘制。**试玩** 会先校验关卡，再进入可移动和推灯的 3D 场景，返回时保留草稿。
- **保存 / 载入** 保存或加载到选定槽位。存档位于 `Saved/VaultLevels/slot-1.json` 至 `slot-3.json`，旧版存档可直接加载。

## 工程结构

`VaultBoard.cpp` 定义关卡和共用规则。3D 渡口由 `VaultCourtyard.cpp`、`VaultSurrounds.cpp` 和 `VaultGeometry.cpp` 生成，石材、符文和水面材质可在 `Content/Art/Materials` 编辑。UE 自动化测试 `RelayVault` 回放七章通关路线，验证机关撤销和编辑器存档往返。

## 美术与字体

界面以渡口手记为主题，使用自制纸纹、灯形朱印和青铜色线框。简体中文标题采用 Noto Serif SC，正文采用 Noto Sans SC。开源字体和 SIL Open Font License 随工程放在 `Content/Slate/Fonts`。[Noto 字体项目](https://github.com/notofonts/noto-cjk)
