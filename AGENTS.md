# 编译指南

## 环境要求

- **Visual Studio 2022 Build Tools** (v145) 或 Visual Studio 2022
- **Windows SDK 10.0** 或更高
- **MSBuild**（随 VS Build Tools 安装）

## 编译命令

### x64（推荐，对应 64 位 TrafficMonitor）

```
msbuild GoldPrice.sln -p:Platform=x64 -p:Configuration=Release
```

输出：`bin\x64\GoldPrice.dll`

### x86（Win32，对应 32 位 TrafficMonitor）

```
msbuild GoldPrice.sln -p:Platform=Win32 -p:Configuration=Release
```

输出：`bin\Win32\GoldPrice.dll`

> 如果在 Git Bash (MSYS2) 中运行，`-p:` 使用 `-p:` 前缀（而非 `/p:`），避免路径转义。

## 编译产物

| 架构 | DLL 路径 |
|------|----------|
| x64  | `bin\x64\GoldPrice.dll` |
| Win32 | `bin\Win32\GoldPrice.dll` |

## 安装

将对应架构的 `GoldPrice.dll` 复制到 TrafficMonitor 目录下的 `plugins\` 文件夹，重启 TrafficMonitor 即可。
