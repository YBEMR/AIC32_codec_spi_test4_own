# uarta_lib README

## 1. 库简介
`uarta_lib` 提供 UART-A 初始化与常用发送接口，适合调试打印、AT 指令发送和串口联调。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

类型说明：
- `Uint16`：16 位无符号整数。
- `Uint32`：32 位无符号整数。

## 2. 头文件与链接方式
- 头文件：`#include "uart.h"`
- 链接库：`uarta_lib.lib`

## 3. 初始化顺序建议
1. `UARTa_Init(baud)`
2. 再调用各类发送函数进行日志或命令输出

## 4. 函数清单

### 4.1 `void UARTa_Init(Uint32 baud);`
- 作用：初始化 UART-A 波特率与串口外设。
- 参数：
  - `baud`：目标波特率（输入），单位 bps，例如 `115200`。
- 返回值：无返回。
- 调用时机：系统初始化阶段调用一次。
- 最小示例：
```c
#include "uart.h"

void uart_init(void)
{
    UARTa_Init(115200);
}
```

### 4.2 `void UARTa_SendByte(int a);`
- 作用：发送单个字节。
- 参数：
  - `a`：要发送的数据（输入），低 8 位有效。
- 返回值：无返回。
- 调用时机：初始化完成后可随时调用；不建议高频在 ISR 中使用。
- 最小示例：
```c
#include "uart.h"

void send_ok_char(void)
{
    UARTa_SendByte('O');
    UARTa_SendByte('K');
}
```

### 4.3 `void UARTa_SendString(char * msg);`
- 作用：发送字符串。
- 参数：
  - `msg`：字符串指针（输入），建议以 `\0` 结尾。
- 返回值：无返回。
- 调用时机：调试打印或发送 AT 命令文本时调用。
- 最小示例：
```c
#include "uart.h"

void log_boot(void)
{
    UARTa_SendString("boot ok\r\n");
}
```

### 4.4 `void UARTa_SendUint16(Uint16 data);`
- 作用：发送 `Uint16` 数值（格式由实现决定）。
- 参数：
  - `data`：16 位无符号数（输入）。
- 返回值：无返回。
- 调用时机：调试数值输出时调用。
- 最小示例：
```c
#include "uart.h"

void log_count(Uint16 cnt)
{
    UARTa_SendUint16(cnt);
}
```

### 4.5 `void UARTa_SendNumber(int32 number);`
- 作用：发送 32 位有符号整型数值。
- 参数：
  - `number`：待发送整数（输入）。
- 返回值：无返回。
- 调用时机：状态值、偏移量等日志输出。
- 最小示例：
```c
#include "uart.h"

void log_offset(int32 offset)
{
    UARTa_SendNumber(offset);
    UARTa_SendString("\r\n");
}
```

### 4.6 `void UARTa_SendHex(Uint16 number);`
- 作用：以十六进制格式发送 16 位数值。
- 参数：
  - `number`：16 位值（输入）。
- 返回值：无返回。
- 调用时机：寄存器/帧头调试打印。
- 最小示例：
```c
#include "uart.h"

void log_hex(Uint16 value)
{
    UARTa_SendHex(value);
    UARTa_SendString("\r\n");
}
```

### 4.7 `void UARTa_SendStringAndNumber(char * msg1, int32 number, char * msg2);`
- 作用：发送“前缀字符串 + 数字 + 后缀字符串”。
- 参数：
  - `msg1`：前缀字符串（输入）。
  - `number`：中间数字（输入）。
  - `msg2`：后缀字符串（输入）。
- 返回值：无返回。
- 调用时机：结构化日志输出时调用。
- 最小示例：
```c
#include "uart.h"

void log_state(int32 state)
{
    UARTa_SendStringAndNumber("state=", state, "\r\n");
}
```

### 4.8 `void UARTa_SendStringAndHex(char * msg1, Uint16 number, char * msg2);`
- 作用：发送“前缀字符串 + 十六进制数 + 后缀字符串”。
- 参数：
  - `msg1`：前缀字符串（输入）。
  - `number`：十六进制值（输入）。
  - `msg2`：后缀字符串（输入）。
- 返回值：无返回。
- 调用时机：SPI/UART 帧内容调试时调用。
- 最小示例：
```c
#include "uart.h"

void log_id(Uint16 id)
{
    UARTa_SendStringAndHex("id=0x", id, "\r\n");
}
```

### 4.9 `void UART_AutoBaud_Test(void);`
- 作用：执行串口自动波特率相关测试流程。
- 参数：无。
- 返回值：无返回。
- 调用时机：串口联调阶段，确认波特率识别能力时调用。
- 最小示例：
```c
#include "uart.h"

void run_autobaud_test(void)
{
    UART_AutoBaud_Test();
}
```

## 5. 常见错误与注意事项
1. 先 `UARTa_Init` 再发送，否则可能无输出。
2. `UARTa_SendString` 传入的字符串要可读且以 `\0` 结尾。
3. 串口打印不建议放在高频 ISR 中，避免影响实时性。

