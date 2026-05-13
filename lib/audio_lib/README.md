# audio_lib README

## 1. 库简介
`audio_lib` 提供 AIC23 音频编解码器初始化、I2C 初始化、WAV 头处理与 AMR 编解码相关能力。  
典型用于“录音 -> 编码 -> 传输 -> 解码 -> 播放”流程中的音频处理环节。

依赖：
- DSP2833x 设备头文件（`DSP2833x_Device.h`、`DSP2833x_Examples.h`）
- AMR/WAV 相关头文件（`tw_amr.h`、`wavreader.h`）

## 2. 头文件与链接方式
- 头文件：`#include "audio.h"`
- 链接库：`audio_lib.lib`

## 3. 初始化顺序建议
1. `I2CA_Init()`
2. `AIC23Init()`
3. 再进入录音/播放或编解码流程

说明：`create_wav_header`、格式转换和 AMR 编解码函数不依赖中断，可在主循环或任务函数中调用。

## 4. 函数清单

### 4.1 `void AIC23Init(void);`
- 作用：初始化 AIC23 音频编解码器寄存器与音频通道。
- 参数：无。
- 返回值：无返回。
- 调用时机：系统初始化阶段调用一次；不建议在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

void audio_hw_init(void)
{
    I2CA_Init();
    AIC23Init();
}
```

### 4.2 `void I2CA_Init(void);`
- 作用：初始化 I2C-A 外设，供 AIC23 配置使用。
- 参数：无。
- 返回值：无返回。
- 调用时机：`AIC23Init()` 之前调用；初始化阶段调用。
- 最小示例：
```c
#include "audio.h"

void board_audio_init(void)
{
    I2CA_Init();
}
```

### 4.3 `void create_wav_header(uint8_t *header, uint32_t data_size);`
- 作用：在缓冲区写入 WAV 文件头信息。
- 参数：
  - `header`：输出缓冲区指针（输出），需可写且有足够空间保存 WAV 头。
  - `data_size`：PCM 数据长度，单位为字节（bytes）。
- 返回值：无返回。
- 调用时机：编码前整理 WAV 数据时调用；不在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

uint8_t wav_buf[60000];
uint32_t pcm_bytes = 32000;

void build_header(void)
{
    create_wav_header(wav_buf, pcm_bytes);
}
```

### 4.4 `void convert_16bit_to_8bit(const int16_t *src, uint8_t *dst, uint32_t num_samples);`
- 作用：将 16 位采样数据转换为 8 位字节流表示。
- 参数：
  - `src`：输入 16 位采样缓冲区（输入），单位为采样点（samples）。
  - `dst`：输出 8 位缓冲区（输出），单位为字节（bytes）。
  - `num_samples`：输入采样点数量（16-bit samples）。
- 返回值：无返回。
- 调用时机：录音采样后、AMR 编码前调用；不在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

int16_t pcm16[160];
uint8_t pcm8[320];

void pack_pcm(void)
{
    convert_16bit_to_8bit(pcm16, pcm8, 160);
}
```

### 4.5 `int16_t amr_encode_wav(const uint8_t *wav_data, uint32_t wav_len, uint8_t *amr_buf, uint16_t amr_buf_size, uint16_t *amr_len);`
- 作用：将 WAV/PCM 字节数据编码为 AMR 数据。
- 参数：
  - `wav_data`：输入 WAV 数据缓冲区（输入），单位为字节。
  - `wav_len`：输入 WAV 数据长度，单位为字节。
  - `amr_buf`：AMR 输出缓冲区（输出）。
  - `amr_buf_size`：`amr_buf` 可用最大长度，单位为字节。
  - `amr_len`：实际编码输出长度（输出），单位为字节。
- 返回值：编码状态码；`0` 通常表示成功，非 `0` 表示失败（以实现为准）。
- 调用时机：完成录音并准备发送前调用；不在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

uint8_t wav_data[60000];
uint8_t amr_data[1500];
uint16_t amr_len = 0;

int16_t encode_once(void)
{
    return amr_encode_wav(wav_data, 60000, amr_data, 1500, &amr_len);
}
```

### 4.6 `int16_t amr_decode_wav(const uint8_t *amr_data, uint16_t amr_len, uint8_t *wav_ptr, uint16_t wav_buf_size, uint16_t *wav_len);`
- 作用：将 AMR 数据解码为 WAV/PCM 字节数据。
- 参数：
  - `amr_data`：AMR 输入缓冲区（输入），单位为字节。
  - `amr_len`：AMR 输入长度，单位为字节。
  - `wav_ptr`：WAV 输出缓冲区（输出）。
  - `wav_buf_size`：`wav_ptr` 最大可写长度，单位为字节。
  - `wav_len`：实际解码输出长度（输出），单位为字节。
- 返回值：解码状态码；`0` 通常表示成功，非 `0` 表示失败（以实现为准）。
- 调用时机：SPI 回包完成后、播放前调用；不在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

uint8_t amr_data[1500];
uint8_t wav_out[60000];
uint16_t wav_len = 0;

int16_t decode_once(void)
{
    return amr_decode_wav(amr_data, 1500, wav_out, 60000, &wav_len);
}
```

### 4.7 `void convert_8bit_to_16bit(const uint8_t *src, uint16_t *dst, uint32_t num_bytes);`
- 作用：将 8 位字节流转换为 16 位采样数据。
- 参数：
  - `src`：输入 8 位数据缓冲区（输入），单位为字节。
  - `dst`：输出 16 位缓冲区（输出），单位为采样点/16-bit 数据。
  - `num_bytes`：输入字节数，单位为字节。
- 返回值：无返回。
- 调用时机：AMR 解码后、送入播放链路前调用；不在 ISR 中调用。
- 最小示例：
```c
#include "audio.h"

uint8_t wav_bytes[320];
uint16_t pcm16[160];

void unpack_pcm(void)
{
    convert_8bit_to_16bit(wav_bytes, pcm16, 320);
}
```

## 5. 常见错误与注意事项
1. `amr_buf_size` / `wav_buf_size` 不足会导致编码或解码失败，必须与业务包长匹配。
2. `num_samples` 与 `num_bytes` 单位不同：一个是采样点，一个是字节，避免混用。
3. 头文件里 `__amr_decoder_create`、`__write_*` 为内部辅助函数，学生侧不建议直接调用。
