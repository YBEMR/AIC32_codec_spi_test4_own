# key_lib README

## 1. 库简介
`key_lib` 提供按键 GPIO 初始化能力，用于“按下开始录音、松开触发编码”等教学流程的输入侧基础配置。

依赖：
- `DSP2833x_Device.h`
- `DSP2833x_Examples.h`

## 2. 头文件与链接方式
- 头文件：`#include "key.h"`
- 链接库：`key_lib.lib`

## 3. 初始化顺序建议
1. 系统启动后调用 `Key_Init()`
2. 再结合 `exint_lib` 或轮询逻辑读取按键事件

## 4. 函数清单

### 4.1 `void Key_Init(void);`
- 作用：初始化按键相关 GPIO（输入方向、上下拉等，具体以实现为准）。
- 参数：无。
- 返回值：无返回。
- 调用时机：初始化阶段调用一次。
- 最小示例：
```c
#include "key.h"

void input_init(void)
{
    Key_Init();
}
```

## 5. 常见错误与注意事项
1. `Key_Init` 只做硬件初始化，不等价于“开始监听事件”。
2. 若配合外部中断使用，请确保 GPIO 复用与 `EXINTx_Init` 对应一致。
3. 按键抖动处理建议在 app 层统一做，保持库职责清晰。

