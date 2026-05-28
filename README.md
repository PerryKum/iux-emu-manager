# emu_manager

ROCKNIX / iux 掌机上的原生小程序（SDL2，640×480），用于在机内管理首页**分组**与**模拟器入口**的显示/隐藏，以及入口所在分组。通过移动 `sections/` 与 `sections/emubak/` 下的文件和目录生效。

## 部署

将仓库内文件拷到 TF 卡（挂载为 `/storage`）对应路径。文本类用 **UTF-8、LF**；`emumanager`、`emumanager.sh` 需 **`chmod +x`**。

| 仓库路径 | 掌机路径 | 说明 |
|----------|----------|------|
| `dist/emumanager` | `/storage/iux/emumanager/emumanager` | 主程序（aarch64 二进制） |
| `packaging/emumanager.sh` | `/storage/iux/emumanager/emumanager.sh` | 启动脚本（首页 `exec` 指向此文件） |
| `packaging/emumanager` | `/storage/iux/sections/applications/emumanager` | 首页「应用」分组入口描述 |
| `packaging/emumanager.png` | `/storage/iux/skins/Default/icons/emumanager.png` | 菜单图标（入口里 `icon=icons/emumanager.png`） |

程序运行时读写的路径：

| 用途 | 掌机路径 |
|------|----------|
| 显示中的分组 | `/storage/iux/sections/<分组名>/` |
| 隐藏的分组（整组目录） | `/storage/iux/sections/emubak/<分组名>/` |
| 隐藏的单个入口 | `/storage/iux/sections/emubak/<入口 id>` |

## 功能概览

### 分组（标签页「分组」）

| 分组 | 目录名 | 整组可隐藏 |
|------|--------|------------|
| 应用 | `applications` | 否（始终显示） |
| 模拟器 | `emulators` | 是 |
| 掌机 | `handheld` | 是 |
| 主机 | `console` | 是 |
| 设置 | `settings` | 否（始终显示） |
| 街机 | `arcade` | 是 |

左右切换：**显示** ↔ **隐藏**。隐藏时会把整个分组目录从 `sections/<名>/` 挪到 `sections/emubak/<名>/`。

### 应用（标签页「应用」）

列出各分组目录下的入口文件，以及 `emubak/` 根目录里单独隐藏的入口。左右切换目标：

- **隐藏**（文件在 `sections/emubak/<入口 id>`）
- 当前为 **显示** 的分组（模拟器 / 掌机 / 主机 / 街机 / 应用 / 设置）

规则摘要：

- **已隐藏的分组**不能作为目标；分组被设为隐藏时，组内可调整的模拟器会在界面上标为「隐藏」。
- 若先把分组隐藏、再在保存前改回显示，因分组隐藏而连带标为隐藏的项会**自动恢复**到该分组（除非你其间手动改过位置）。
- **应用**、**设置**分组内的入口不可单独隐藏。
- 以下入口固定，不可移动：`02usbunmount（卸载USB）`、`03usbmount（挂载USB）`、`emulationstation（切换到es前端）`、`update（系统更新）`、`ra（RA 入口）`、`ppsspp（PSP 模拟器入口）`、`explor（文件管理器）`、`emumanager（本应用）`。

### 保存与退出

| 操作 | 效果 |
|------|------|
| **START** → 确认（A） | 将待保存的变更写入存储卡（移动文件/目录），然后退出 |
| **START** → 取消（B） | 关闭对话框，继续编辑 |
| **B** 直接退出 | 不写盘，本次所有改动作废 |

## 按键

| 按键 | 作用 |
|------|------|
| 方向键 / 左摇杆 | 移动焦点；左右切换当前行的状态 |
| L1 / R1 | 在「分组」「应用」标签页间切换 |
| X / Y | 翻页 |
| START | 保存并退出（需确认） |
| B / Select | 不保存退出；或在确认框中取消 |

## 在 Windows 上编译

需要 [Zig](https://ziglang.org/)（建议 0.14.x）在 `PATH` 中，或放在 `.tools/zig/`（已 gitignore，可自行下载解压）。

```powershell
cd emu_manager
.\scripts\build_zig.ps1
```

产物：`dist/emumanager`（`aarch64-linux-gnu`）。

首次构建会执行 `scripts/setup_sysroot.ps1`：从 Debian 下载 arm64 的 SDL2 开发包，解压到 `sysroot/`。`sysroot_pkgs/` 仅缓存 `.deb`，可删后重建。

## 在 Linux 上编译（可选）

设备本机或带交叉工具链的 PC：

```bash
make native          # 本机 aarch64
make aarch64         # 交叉：aarch64-linux-gnu-gcc + pkg-config SDL2
```

## 项目结构

```
emu_manager/
├── include/          # 头文件
├── src/              # main / scan / fs_util / ui
├── packaging/        # 其他必要文件（入口文件、应用 icon 等）
├── dist/             # 编译输出
└── Makefile          # Linux / 设备上 make
```

## 许可证

本项目采用 [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html)（GPL-3.0）。完整条文见仓库根目录 [`LICENSE`](LICENSE)。
