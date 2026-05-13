# codec_service_lib README

## 1. 库简介
`codec_service_lib` 是面向教学流程的服务层库，封装了“录音缓存管理 -> AMR 编码 -> SPI 两次交换 -> AMR 解码 -> 播放采样输出”的可复用步骤。  
该库不包含 ISR 入口和 `main`，推荐由 `student_codec_app.c` 在主循环/状态机中调度。

依赖：
- `audio_lib`（AMR 编解码与格式处理）
- `spi_lib`（SPI 收发）
- `DSP2833x_Device.h`、`DSP2833x_Examples.h`

类型说明：
- `Uint16`：16 位无符号整数。
- `int16_t`：16 位有符号整数（`stdint.h`）。

## 2. 头文件与链接方式
- 头文件：`#include "codec_service.h"`
- 链接库：`codec_service_lib.lib`

状态码：
- `CODEC_SERVICE_OK`：`0`
- `CODEC_SERVICE_PLAY_DONE`：`1`
- `CODEC_SERVICE_ERR_NO_RECORD`：`-1`
- `CODEC_SERVICE_ERR_ENCODE`：`-2`
- `CODEC_SERVICE_ERR_DECODE`：`-3`
- `CODEC_SERVICE_ERR_LENGTH`：`-4`

## 3. 初始化顺序建议
1. `codec_service_reset()`
2. 按键按下后调用 `codec_service_start_record()`
3. 采样中断或采样循环中反复调用 `codec_service_record_sample()`
4. 按键松开后调用 `codec_service_encode_recorded()`
5. 调用 `codec_service_spi_exchange_first()` / `codec_service_spi_exchange_second()`
6. 调用 `codec_service_decode_received()`
7. 播放 ISR 或播放循环中反复调用 `codec_service_get_play_sample()`

## 4. 函数清单

### 4.1 `void codec_service_reset(void);`
- 作用：重置服务内部状态、长度计数与偏移。
- 参数：无。
- 返回值：无返回。
- 调用时机：系统初始化或每轮流程开始前调用。
- 最小示例：
```c
#include "codec_service.h"

void app_reset_flow(void)
{
    codec_service_reset();
}
```

### 4.2 `void codec_service_start_record(void);`
- 作用：开始新一轮录音，准备接收采样点。
- 参数：无。
- 返回值：无返回。
- 调用时机：按键按下事件到来时调用。
- 最小示例：
```c
#include "codec_service.h"

void on_key_press(void)
{
    codec_service_start_record();
}
```

### 4.3 `int16_t codec_service_record_sample(int16_t sample);`
- 作用：写入一个录音采样点到内部录音缓冲区。
- 参数：
  - `sample`：输入采样点（输入），单位为 16-bit PCM 采样点。
- 返回值：状态码；通常 `CODEC_SERVICE_OK` 成功，长度超限时返回错误码。
- 调用时机：采样 ISR 或采样循环中高频调用。
- 最小示例：
```c
#include "codec_service.h"

interrupt void mcbsp_rx_isr(void)
{
    int16_t s = (int16_t)McbspaRegs.DRR1.all;
    codec_service_record_sample(s);
}
```

### 4.4 `Uint16 codec_service_get_record_count(void);`
- 作用：获取当前已录制采样点数。
- 参数：无。
- 返回值：已录制采样点数量（单位：samples）。
- 调用时机：录音阶段用于状态显示或结束条件判断。
- 最小示例：
```c
#include "codec_service.h"

Uint16 rec_cnt;
void poll_record_count(void)
{
    rec_cnt = codec_service_get_record_count();
}
```

### 4.5 `int16_t codec_service_encode_recorded(void);`
- 作用：将当前录音数据编码为 AMR。
- 参数：无。
- 返回值：状态码；成功返回 `CODEC_SERVICE_OK`，失败返回负值错误码。
- 调用时机：录音结束后调用；不在 ISR 中调用。
- 最小示例：
```c
#include "codec_service.h"

int16_t do_encode(void)
{
    return codec_service_encode_recorded();
}
```

### 4.6 `int16_t codec_service_spi_exchange_first(void);`
- 作用：执行第一阶段 SPI 收发（发送编码结果并接收回包或中间数据）。
- 参数：无。
- 返回值：状态码；成功 `CODEC_SERVICE_OK`，失败负值。
- 调用时机：编码成功后、主流程中调用；不在 ISR 中调用。
- 最小示例：
```c
#include "codec_service.h"

int16_t do_spi_round1(void)
{
    return codec_service_spi_exchange_first();
}
```

### 4.7 `int16_t codec_service_spi_exchange_second(void);`
- 作用：执行第二阶段 SPI 收发（按当前协议完成后续交换）。
- 参数：无。
- 返回值：状态码；成功 `CODEC_SERVICE_OK`，失败负值。
- 调用时机：第一阶段成功后调用；主流程中执行。
- 最小示例：
```c
#include "codec_service.h"

int16_t do_spi_round2(void)
{
    return codec_service_spi_exchange_second();
}
```

### 4.8 `int16_t codec_service_decode_received(void);`
- 作用：将 SPI 接收到的 AMR 数据解码为可播放 WAV/PCM 数据。
- 参数：无。
- 返回值：状态码；成功 `CODEC_SERVICE_OK`，失败负值。
- 调用时机：SPI 两阶段交换完成后调用；不在 ISR 中调用。
- 最小示例：
```c
#include "codec_service.h"

int16_t do_decode(void)
{
    return codec_service_decode_received();
}
```

### 4.9 `int16_t codec_service_get_play_sample(Uint16 *sample);`
- 作用：按顺序取出一个播放采样点。
- 参数：
  - `sample`：输出采样点地址（输出），单位为 16-bit 采样点。
- 返回值：
  - `CODEC_SERVICE_OK`：成功取到采样点；
  - `CODEC_SERVICE_PLAY_DONE`：播放数据已取完；
  - 其他负值：异常。
- 调用时机：播放 ISR 或播放循环中高频调用。
- 最小示例：
```c
#include "codec_service.h"

interrupt void mcbsp_tx_isr(void)
{
    Uint16 s = 0;
    if (codec_service_get_play_sample(&s) == CODEC_SERVICE_OK)
    {
        McbspaRegs.DXR1.all = s;
    }
}
```

### 4.10 `Uint16 codec_service_get_amr_len(void);`
- 作用：获取本端编码后的 AMR 长度。
- 参数：无。
- 返回值：AMR 数据长度（单位：字节）。
- 调用时机：编码后用于日志或发包长度确认。
- 最小示例：
```c
#include "codec_service.h"

Uint16 amr_len;
void log_amr_len(void)
{
    amr_len = codec_service_get_amr_len();
}
```

### 4.11 `Uint16 codec_service_get_received_amr_len(void);`
- 作用：获取 SPI 接收后的 AMR 长度。
- 参数：无。
- 返回值：接收 AMR 长度（单位：字节）。
- 调用时机：SPI 收发后、解码前用于长度校验。
- 最小示例：
```c
#include "codec_service.h"

Uint16 rx_len;
void check_rx_len(void)
{
    rx_len = codec_service_get_received_amr_len();
}
```

### 4.12 `Uint16 codec_service_get_wav_len(void);`
- 作用：获取解码输出的 WAV/PCM 数据长度。
- 参数：无。
- 返回值：输出长度（单位：字节）。
- 调用时机：解码成功后用于播放范围控制。
- 最小示例：
```c
#include "codec_service.h"

Uint16 wav_len;
void log_wav_len(void)
{
    wav_len = codec_service_get_wav_len();
}
```

### 4.13 `Uint16 codec_service_get_play_offset(void);`
- 作用：获取当前播放偏移位置。
- 参数：无。
- 返回值：播放偏移量（单位以实现为准，通常为样本索引或字节偏移）。
- 调用时机：播放阶段状态观察。
- 最小示例：
```c
#include "codec_service.h"

Uint16 offset;
void poll_play_offset(void)
{
    offset = codec_service_get_play_offset();
}
```

### 4.14 `const uint8_t *codec_service_get_amr_buffer(void);`
- 作用：获取内部 AMR 编码缓冲区只读指针。
- 参数：无。
- 返回值：AMR 缓冲区首地址（只读）。
- 调用时机：编码后用于调试或额外传输流程。
- 最小示例：
```c
#include "codec_service.h"

const uint8_t *amr_ptr;
void fetch_amr_ptr(void)
{
    amr_ptr = codec_service_get_amr_buffer();
}
```

### 4.15 `const uint8_t *codec_service_get_spi_rx_buffer(void);`
- 作用：获取内部 SPI 接收缓冲区只读指针。
- 参数：无。
- 返回值：SPI RX 缓冲区首地址（只读）。
- 调用时机：SPI 收发后用于调试查看。
- 最小示例：
```c
#include "codec_service.h"

const uint8_t *rx_ptr;
void fetch_rx_ptr(void)
{
    rx_ptr = codec_service_get_spi_rx_buffer();
}
```

### 4.16 `const uint8_t *codec_service_get_wav_buffer(void);`
- 作用：获取内部解码后的 WAV/PCM 缓冲区只读指针。
- 参数：无。
- 返回值：WAV 缓冲区首地址（只读）。
- 调用时机：解码后用于播放链路或调试。
- 最小示例：
```c
#include "codec_service.h"

const uint8_t *wav_ptr;
void fetch_wav_ptr(void)
{
    wav_ptr = codec_service_get_wav_buffer();
}
```

## 5. 常见错误与注意事项
1. 录音未开始或样本数为 0 就调用编码，会得到 `CODEC_SERVICE_ERR_NO_RECORD`。
2. 长度单位要区分清楚：采样点（samples）与字节（bytes）不能混用。
3. `codec_service_encode_recorded`、`codec_service_decode_received`、SPI 两阶段交换属于重处理，放主循环；`record_sample`、`get_play_sample` 可放采样/播放 ISR。

