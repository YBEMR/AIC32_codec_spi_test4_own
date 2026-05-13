# led_lib README

## 1. 库简介
`led_lib` 提供板载 LED GPIO 初始化能力，并在头文件中提供 LED 开关/翻转宏，便于状态指示。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

## 2. 头文件与链接方式
- 头文件：`#include "leds.h"`
- 链接库：`led_lib.lib`

## 3. 初始化顺序建议
1. 上电后先调用 `LED_Init()`
2. 再在主循环或 ISR 中按需使用 `LEDx_ON/OFF/TOGGLE` 宏

## 4. 函数清单

### 4.1 `void LED_Init(void);`
- 作用：初始化 LED 相关 GPIO 方向与默认状态。
- 参数：无。
- 返回值：无返回。
- 调用时机：系统初始化阶段调用一次。
- 最小示例：
```c
#include "leds.h"

void board_led_init(void)
{
    LED_Init();
    LED1_OFF;
}
```

## 5. 常见错误与注意事项
1. 先 `LED_Init` 再使用 `LEDx_*` 宏，否则 GPIO 方向可能未配置。
2. `LEDx_ON/OFF/TOGGLE` 是寄存器宏，不是函数调用，不要写成带参数形式。
3. 频繁翻转 LED 时建议在定时器节拍中触发，避免主循环阻塞。

