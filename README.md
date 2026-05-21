# AIC32 Codec SPI 工程编译说明

本工程提供两个编译入口：

- 学生使用根目录 `Makefile`，在 VSCode 或终端中直接运行 `gmake all`。
- `Debug/` 目录保留给 CCS 自动生成工程使用，里面的 `.mk` 文件可能包含本机路径，学生不要进入 `Debug/` 编译。

## 1. 准备编译环境

学生需要安装对应系统版本的 TI C2000 Code Generation Tools，并设置 `CG_TOOL_ROOT` 指向编译器根目录。Windows 推荐使用正斜杠路径。

Windows PowerShell 示例：



```powershell
$env:CG_TOOL_ROOT="<CCS_INSTALL>/tools/compiler/ti-cgt-c2000_22.6.0.LTS"
$env:PATH="$env:CG_TOOL_ROOT/bin;<CCS_INSTALL>/utils/bin;$env:PATH"
```
例如
$env:CG_TOOL_ROOT="F:\ti\CCS8.3\ccsv8\tools\compiler\ti-cgt-c2000_22.6.0.LTS"
$env:PATH="$env:CG_TOOL_ROOT/bin;F:\ti\CCS8.3\ccsv8\utils\bin;$env:PATH"

Linux bash 示例：

```bash
export CG_TOOL_ROOT="$HOME/ti/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS"
export PATH="$CG_TOOL_ROOT/bin:$PATH"
```

也可以在系统环境变量中永久设置 `CG_TOOL_ROOT`。

## 2. 编译学生主工程

在工程根目录运行：

```powershell
gmake all
```

Linux 下如果没有 `gmake` 命令，直接使用系统 GNU Make 也可以：

```bash
make all
```

生成结果：

```text
build/debug/AIC32_codec_spi_test4.out
build/debug/AIC32_codec_spi_test4.txt
```

清理输出：

```powershell
gmake clean
```

Linux 下对应为：

```bash
make clean
```

VSCode 中可以直接运行默认构建任务 `gmake all`。

## 3. 工程结构

- `Makefile`：学生用便携式构建入口。
- `user/student_codec_app.c`：学生 app 入口。
- `source/`：F28335 启动、外设初始化、链接命令文件和 `IQmath_fpu32.lib`。
- `lib/release/include`：app 编译使用的库公开头文件。
- `lib/release/lib`：app 链接使用的预编译 `.lib`。
- `third_party/DSP2833x`：随工程分发的 DSP2833x common/header 头文件。
- `student_release/`：给学生发布用的头文件、库和示例代码镜像。

## 4. 教师维护库

本工程已经整理为 `lib + app` 分离结构。学生 app 平常只链接 `lib/release/lib` 里的 `.lib`，不需要重编底层库。

教师修改库源码后，可以继续使用 `lib/Makefile` 重新构建并同步发布包：

```powershell
cd lib
gmake all
gmake student-release
```

同步后目录为：

```text
student_release/include
student_release/lib
```

如果库工程的 `Debug/` 生成文件包含本机路径，只影响教师重编库，不影响学生从工程根目录运行 `gmake all`。

## 5. ABI 注意事项

预编译库面向 TMS320F28335、COFF ABI、TI C2000 编译器 `22.6.0.LTS`。不要和 EABI 或其他版本工具链混用。

## 6. 下载程序
python .\read_data1.py -p COM16 -w .\build\debug\AIC32_codec_spi_test4_own.txt