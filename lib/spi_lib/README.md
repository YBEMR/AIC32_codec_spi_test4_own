# spi_lib README

## 1. 库简介
`spi_lib` 提供 DSP 侧 SPI 初始化、就绪信号配置和收发接口，用于与 MCU 进行数据交换。  
典型用于编码后数据发送、接收回环数据的链路。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

类型说明：
- `Uint16`：TI C2000 平台 16 位无符号整数类型。
- `PINT`：中断函数指针类型（来自 DSP2833x 头文件）。

## 2. 头文件与链接方式
- 头文件：`#include "spi.h"`
- 链接库：`spi_lib.lib`

## 3. 初始化顺序建议
1. `spi_init()`
2. `spi_ready_init(isr)`（如需要流控/就绪中断）
3. 在业务阶段调用 `spi_send_and_receive` 或 `spi_send_bulk`

## 4. 函数清单

### 4.1 `void spi_init(void);`
- 作用：初始化 SPI 外设参数和 GPIO 复用配置。
- 参数：无。
- 返回值：无返回。
- 调用时机：系统初始化阶段调用一次；不在 ISR 中调用。
- 最小示例：
```c
#include "spi.h"

void spi_hw_init(void)
{
    spi_init();
}
```

### 4.2 `void spi_ready_init(PINT isr);`
- 作用：初始化 SPI 就绪/流控相关 GPIO 与中断入口。
- 参数：
  - `isr`：中断服务函数指针（输入），用于注册就绪信号 ISR。
- 返回值：无返回。
- 调用时机：`spi_init()` 后调用；初始化阶段调用。
- 最小示例：
```c
#include "spi.h"

interrupt void spi_ready_isr(void)
{
    // 只做标志置位
}

void init_ready_irq(void)
{
    spi_ready_init(spi_ready_isr);
}
```

### 4.3 `void spi_send_and_receive(const Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);`
- 作用：按给定长度执行一次 SPI 发送并接收。
- 参数：
  - `send_buffer`：发送缓冲区（输入），单位为 `Uint16` 元素。
  - `receive_buffer`：接收缓冲区（输出），单位为 `Uint16` 元素。
  - `length`：收发长度，单位为 `Uint16` 元素个数。
- 返回值：无返回。
- 调用时机：业务循环中调用；不建议在 ISR 中执行整包传输。
- 最小示例：
```c
#include "spi.h"

Uint16 tx_buf[1500];
Uint16 rx_buf[1500];

void exchange_packet(void)
{
    spi_send_and_receive(tx_buf, rx_buf, 1500);
}
```

### 4.4 `void spi_send_bulk(Uint16* buffer, Uint16 *receive_buffer, Uint16 length);`
- 作用：批量发送并接收数据（接口语义与工程实现保持一致）。
- 参数：
  - `buffer`：发送缓冲区（输入），单位为 `Uint16` 元素。
  - `receive_buffer`：接收缓冲区（输出），单位为 `Uint16` 元素。
  - `length`：收发长度，单位为 `Uint16` 元素个数。
- 返回值：无返回。
- 调用时机：大包传输场景；不建议在 ISR 中调用。
- 最小示例：
```c
#include "spi.h"

Uint16 tx_data[256];
Uint16 rx_data[256];

void send_bulk_once(void)
{
    spi_send_bulk(tx_data, rx_data, 256);
}
```

### 4.5 `int16 spi_lookback_test(Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length);`
- 作用：执行 SPI 回环/连通性测试。
- 参数：
  - `send_buffer`：发送测试数据缓冲区（输入）。
  - `receive_buffer`：接收测试数据缓冲区（输出）。
  - `length`：测试长度，单位为 `Uint16` 元素个数。
- 返回值：测试结果状态码；通常 `0` 成功，非 `0` 失败（以实现为准）。
- 调用时机：联调与自检阶段，正式业务前可调用一次。
- 最小示例：
```c
#include "spi.h"

Uint16 tx_test[32];
Uint16 rx_test[32];

int16 do_spi_selftest(void)
{
    return spi_lookback_test(tx_test, rx_test, 32);
}
```

## 5. 常见错误与注意事项
1. `length` 单位是 `Uint16` 元素数量，不是字节数。
2. 传输函数应在主流程调用，ISR 内只做事件置位，避免阻塞中断。
3. 流控链路存在时，先完成 `spi_ready_init` 再进行正式收发。

