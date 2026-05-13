# timer0_lib README

## 1. 库简介
`timer0_lib` 提供 CPU Timer0 初始化接口，用于周期中断驱动采样、心跳或状态扫描。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

类型说明：
- `PINT`：中断函数指针类型。

## 2. 头文件与链接方式
- 头文件：`#include "timer.h"`
- 链接库：`timer0_lib.lib`

## 3. 初始化顺序建议
1. 配置中断服务函数
2. 调用 `TIM0_Init(Freq, Period, isr)`
3. 使能全局中断并进入主循环

## 4. 函数清单

### 4.1 `void TIM0_Init(float Freq, float Period, PINT isr);`
- 作用：初始化 Timer0 周期中断。
- 参数：
  - `Freq`：CPU 主频，单位 MHz（工程注释示例为 `150`）。
  - `Period`：定时周期，单位 us（微秒）。
  - `isr`：Timer0 中断服务函数指针（输入）。
- 返回值：无返回。
- 调用时机：系统初始化阶段调用；不在 ISR 中调用。
- 最小示例：
```c
#include "timer.h"

interrupt void cpu_timer0_isr(void)
{
    // 置位采样标志
}

void timer_init(void)
{
    TIM0_Init(150.0f, 125.0f, cpu_timer0_isr);
}
```

## 5. 常见错误与注意事项
1. `Freq` 和 `Period` 单位要对应，`Period` 是微秒不是毫秒。
2. ISR 中只做轻量逻辑（置标志/搬运），重处理放主循环。
3. 确保 PIE/CPU 中断组与 `isr` 配置一致。

