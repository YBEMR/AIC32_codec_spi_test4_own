# exint_lib README

## 1. 库简介
`exint_lib` 提供外部中断 EXINT1/EXINT2 初始化接口，常用于按键边沿触发、握手信号触发等场景。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

类型说明：
- `PINT`：中断函数指针类型。

## 2. 头文件与链接方式
- 头文件：`#include "exint.h"`
- 链接库：`exint_lib.lib`

## 3. 初始化顺序建议
1. 先完成 GPIO 方向与复用配置（如由其他模块负责）。
2. 注册 ISR 并调用 `EXINT1_Init` 或 `EXINT2_Init`。
3. 在 ISR 中仅上报事件，业务流程放主循环处理。

## 4. 函数清单

### 4.1 `void EXINT1_Init(PINT ISR_FUNC);`
- 作用：初始化外部中断 1 并绑定 ISR。
- 参数：
  - `ISR_FUNC`：外部中断 1 的服务函数指针（输入）。
- 返回值：无返回。
- 调用时机：初始化阶段调用一次。
- 最小示例：
```c
#include "exint.h"

volatile Uint16 key_release_flag = 0;

interrupt void exint1_isr(void)
{
    key_release_flag = 1;
}

void exti_init(void)
{
    EXINT1_Init(exint1_isr);
}
```

### 4.2 `void EXINT2_Init(PINT ISR_FUNC);`
- 作用：初始化外部中断 2 并绑定 ISR。
- 参数：
  - `ISR_FUNC`：外部中断 2 的服务函数指针（输入）。
- 返回值：无返回。
- 调用时机：初始化阶段调用一次。
- 最小示例：
```c
#include "exint.h"

volatile Uint16 sync_flag = 0;

interrupt void exint2_isr(void)
{
    sync_flag = 1;
}

void exti2_init(void)
{
    EXINT2_Init(exint2_isr);
}
```

## 5. 常见错误与注意事项
1. ISR 函数签名需符合 C2000 中断函数要求（`interrupt void`）。
2. 外部中断配置需与硬件引脚电平/边沿一致。
3. ISR 内不要做编码/解码等重计算，避免中断阻塞。

