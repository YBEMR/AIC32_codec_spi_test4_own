# AIC32 Codec SPI 学生发布包

这个目录用于给学生工程接入 AIC32 录音、AMR 编码、SPI 往返、AMR 解码和播放流程。

## 目录结构

- `include/`：公开头文件，只暴露学生需要调用的 API。
- `lib/`：使用 TI C2000 编译器 `ti-cgt-c2000_22.6.0.LTS` 生成的 COFF `.lib`。
- `examples/student_codec_app.c`：显式状态机示例应用。

## 学生工程接入方式

1. 在 CCS 工程的 Include Options 中加入：
   - `student_release/include`
   - DSP2833x common/header 的 include 路径
2. 在 Linker File Search Path 中加入：
   - `student_release/lib`
3. 在 Linker Libraries 中加入：
   - `codec_service_lib.lib`
   - `audio_lib.lib`
   - `spi_lib.lib`
   - `exint_lib.lib`
   - `timer0_lib.lib`
   - `led_lib.lib`
   - `uarta_lib.lib`
   - `libc.a`
4. 工程中只放自己的 app 源码，例如 `student_codec_app.c`，不要再加入底层驱动 `.c` 文件。

## 示例流程

`student_codec_app.c` 保留了教学用显式状态机：

1. 按下按键进入录音状态。
2. 松开按键后进入编码状态。
3. 编码完成后执行第一次 SPI 传输。
4. 等待 MCU 流控引脚中断后执行第二次 SPI 传输。
5. 校验返回 AMR 长度，解码为 PCM16 样本。
6. 在 McBSP 中断里播放解码后的音频。

## 教师维护方式

底层库源码保留在工程根目录的 `lib/` 中。修改某个模块后，重新编译对应库，再同步到本目录的 `lib/` 即可。

新增的流程库是：

- `lib/codec_service_lib`

它负责大缓冲区、录音样本缓存、编码、两次 SPI 交换、解码和播放取样；ISR 和主状态机仍在 app 层，方便学生阅读。

## ABI 注意事项

这些 `.lib` 面向 TMS320F28335、COFF ABI、TI C2000 编译器 `22.6.0.LTS`。不要和 EABI 或其他版本工具链混用。
