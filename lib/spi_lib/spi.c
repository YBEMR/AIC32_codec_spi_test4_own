#include "spi.h"

#if (SPI_PTT_DSP_REQ_GPIO < 48U) || (SPI_PTT_DSP_REQ_GPIO > 63U) || \
    (SPI_PTT_SPI_READY_GPIO < 48U) || (SPI_PTT_SPI_READY_GPIO > 63U) || \
    (SPI_PTT_DATA_READY_GPIO < 48U) || (SPI_PTT_DATA_READY_GPIO > 63U)
#error "PTT GPIO helpers currently support GPIO48..GPIO63 only."
#endif

/**
 * @brief 计算 GPIO48..GPIO63 在 GPB 数据寄存器中的 bit mask。
 *
 * F28335 的 GPIO48..GPIO63 位于 GPB[16..31]，PTT 三线握手默认都放在
 * 这一段，后续实际飞线只需要改 spi.h 中的 GPIO 宏。
 *
 * @param gpio GPIO 编号，范围必须是 48..63。
 *
 * @return Uint32 对应 GPB 寄存器中的 bit mask。
 */
static Uint32 spi_ptt_gpb_mask(Uint16 gpio)
{
    return ((Uint32)1UL << (gpio - 32U));
}

/**
 * @brief 计算 GPIO48..GPIO63 在 GPBMUX2/GPBQSEL2 中的移位。
 *
 * GPBMUX2 和 GPBQSEL2 每个 GPIO 占 2 bit，因此需要按 GPIO 编号换算
 * 对应字段的位置。
 *
 * @param gpio GPIO 编号，范围必须是 48..63。
 *
 * @return Uint16 对应 2-bit 字段的左移位数。
 */
static Uint16 spi_ptt_gpb48_63_shift(Uint16 gpio)
{
    return (Uint16)((gpio - 48U) * 2U);
}

/**
 * @brief 配置一个 GPIO48..GPIO63 为普通 GPIO 输入或输出。
 *
 * 该 helper 只服务 PTT 三线握手，避免每次换飞线都去改 bitfield 名称。
 *
 * @param gpio GPIO 编号，范围必须是 48..63。
 * @param is_output 非 0 表示配置为输出，0 表示配置为输入。
 * @param input_filter 非 0 表示输入脚使用 6-cycle qualification 滤波。
 *
 * @return void
 */
static void spi_ptt_config_gpb48_63(Uint16 gpio, Uint16 is_output, Uint16 input_filter)
{
    Uint32 mask;
    Uint16 shift;

    mask = spi_ptt_gpb_mask(gpio);
    shift = spi_ptt_gpb48_63_shift(gpio);

    GpioCtrlRegs.GPBMUX2.all &= ~((Uint32)3UL << shift);
    GpioCtrlRegs.GPBPUD.all &= ~mask;

    if (is_output != 0U) {
        GpioCtrlRegs.GPBDIR.all |= mask;
    } else {
        GpioCtrlRegs.GPBDIR.all &= ~mask;
        if (input_filter != 0U) {
            GpioCtrlRegs.GPBQSEL2.all &= ~((Uint32)3UL << shift);
            GpioCtrlRegs.GPBQSEL2.all |= ((Uint32)2UL << shift);
        }
    }
}

void spi_init()
{
	EALLOW;
	SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1; // Enable SPIA clock
	EDIS;

	InitSpiaGpio();
	SpiaRegs.SPICCR.all =0x000F;	             			// Reset on, rising edge, 16-bit char bits
	                                             			// 0x000F corresponds to Rising Edge, 0x004F corresponds to Falling Edge
	SpiaRegs.SPICTL.all =0x0006;    		     			// Enable master mode, normal phase,

	// enable talk, and SPI int disabled.
//	SpiaRegs.SPIBRR =0x007F;
	SpiaRegs.SPIBRR =0x0025;
    SpiaRegs.SPICCR.all =0x008F;		         			// Relinquish SPI from Reset
    SpiaRegs.SPIPRI.bit.FREE = 1;                			// Set so breakpoints don't disturb xmission
}

void spi_ready_init(PINT isr)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;    // Enable GPIO input clock
    EDIS;

    EALLOW;
    // GPIO50 Configuration
    GpioCtrlRegs.GPBMUX2.bit.GPIO50 = 0;
    GpioCtrlRegs.GPBDIR.bit.GPIO50 = 0;         // Input
    GpioCtrlRegs.GPBPUD.bit.GPIO50 = 0;         // Enable pull-up

    // Add filtering to avoid false triggering
    GpioCtrlRegs.GPBQSEL2.bit.GPIO50 = 2;       // 6-cycle filter
    GpioCtrlRegs.GPBCTRL.bit.QUALPRD2 = 0xFF;   // Increase sampling interval
    EDIS;

    EALLOW;
    // --- XINT3 Mapping ---
    // XINT3 for F28335 corresponds to PortB, need to subtract 32 from the value
    GpioIntRegs.GPIOXINT3SEL.bit.GPIOSEL = 50 - 32;
    EDIS;

    EALLOW;
    PieVectTable.XINT3 = isr;
    EDIS;

    // --- Enable PIE Group 12 ---
    // Clear flag first to prevent entering interrupt immediately after enabling
    PieCtrlRegs.PIEIFR12.bit.INTx1 = 0;
    PieCtrlRegs.PIEIER12.bit.INTx1 = 1;

    // Trigger mode
    XIntruptRegs.XINT3CR.bit.POLARITY = 1;      // 1：rising edge, 0：falling edge，3：both edges
    XIntruptRegs.XINT3CR.bit.ENABLE = 1;

    // Enable CPU INT12
    IER |= M_INT12;
}

/**
 * @brief 初始化 PTT 三线握手 GPIO。
 *
 * GPIO51 作为 DSP_REQ 输出，通知 Art-Pi 准备一次 SPI block transaction；
 * GPIO50 作为 SPI_READY 输入，表示 Art-Pi 已经 arm 好 SPI slave DMA；
 * GPIO52 作为 DATA_READY 输入，预留给后续 Art-Pi 通知有完整 session 可读。
 *
 * @return void
 */
void spi_ptt_gpio_init(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.GPIOINENCLK = 1;
    EDIS;

    EALLOW;
    spi_ptt_config_gpb48_63(SPI_PTT_SPI_READY_GPIO, 0U, 1U);
    spi_ptt_config_gpb48_63(SPI_PTT_DSP_REQ_GPIO, 1U, 0U);
    spi_ptt_config_gpb48_63(SPI_PTT_DATA_READY_GPIO, 0U, 1U);
    EDIS;

    GpioDataRegs.GPBCLEAR.all = spi_ptt_gpb_mask(SPI_PTT_DSP_REQ_GPIO);
}

/**
 * @brief 设置 DSP_REQ 握手线电平。
 *
 * DSP 发起一次 PTT SPI block transaction 前拉高 DSP_REQ，等本次 SPI
 * 传输结束后拉低。Art-Pi 侧据此执行 slave DMA arm 和 SPI_READY 通知。
 *
 * @param level 非 0 表示拉高 DSP_REQ，0 表示拉低 DSP_REQ。
 *
 * @return void
 */
void spi_ptt_set_dsp_req(Uint16 level)
{
    if (level != 0U) {
        GpioDataRegs.GPBSET.all = spi_ptt_gpb_mask(SPI_PTT_DSP_REQ_GPIO);
    } else {
        GpioDataRegs.GPBCLEAR.all = spi_ptt_gpb_mask(SPI_PTT_DSP_REQ_GPIO);
    }
}

/**
 * @brief 读取 SPI_READY 握手线。
 *
 * SPI_READY 由 Art-Pi 输出，高电平表示本次 SPI slave DMA 已经准备好，
 * DSP 可以开始输出 SPI clock。
 *
 * @return Uint16 非 0 表示 SPI_READY 为高电平。
 */
Uint16 spi_ptt_is_spi_ready(void)
{
    return (Uint16)((GpioDataRegs.GPBDAT.all & spi_ptt_gpb_mask(SPI_PTT_SPI_READY_GPIO)) != 0UL);
}

/**
 * @brief 读取 DATA_READY 握手线。
 *
 * DATA_READY 由 Art-Pi 输出，后续用于通知 DSP 有业务事件或完整 session
 * 可以读取；本阶段只提供读取封装，不接主状态机。
 *
 * @return Uint16 非 0 表示 DATA_READY 为高电平。
 */
Uint16 spi_ptt_is_data_ready(void)
{
    return (Uint16)((GpioDataRegs.GPBDAT.all & spi_ptt_gpb_mask(SPI_PTT_DATA_READY_GPIO)) != 0UL);
}

/**
 * @brief 阻塞等待 SPI_READY 变为高电平。
 *
 * 该等待运行在普通流程中，不在 ISR 中执行。timeout_loop 用软件循环计数，
 * 主要用于板级联调时避免 Art-Pi 未启动或飞线错误导致 DSP 永久卡死。
 *
 * @param timeout_loop 最大轮询次数，设置为 0 表示不等待直接检查一次。
 *
 * @return int16 SPI_PTT_OK 表示等到 ready，SPI_PTT_ERR_TIMEOUT 表示超时。
 */
int16 spi_ptt_wait_spi_ready(Uint32 timeout_loop)
{
    Uint32 count;

    for (count = 0; count <= timeout_loop; count++) {
        if (spi_ptt_is_spi_ready() != 0U) {
            return SPI_PTT_OK;
        }
    }

    return SPI_PTT_ERR_TIMEOUT;
}

/**
 * @brief 阻塞等待 DATA_READY 变为高电平。
 *
 * 当前阶段 DATA_READY 只作为后续 session 下载通知的预留接口，先提供
 * 与 SPI_READY 一致的超时等待语义。
 *
 * @param timeout_loop 最大轮询次数，设置为 0 表示不等待直接检查一次。
 *
 * @return int16 SPI_PTT_OK 表示等到 ready，SPI_PTT_ERR_TIMEOUT 表示超时。
 */
int16 spi_ptt_wait_data_ready(Uint32 timeout_loop)
{
    Uint32 count;

    for (count = 0; count <= timeout_loop; count++) {
        if (spi_ptt_is_data_ready() != 0U) {
            return SPI_PTT_OK;
        }
    }

    return SPI_PTT_ERR_TIMEOUT;
}

void spi_send_and_receive(const Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length)
{
    Uint16 sent_count = 0;
    Uint16 recv_count = 0;

    // Enable TX FIFO
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 1;

    // Release RX FIFO reset
    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 1;

    while ((sent_count < length) || (recv_count < length))
    {
        // Fill TX FIFO, max 16 data words
        while ((SpiaRegs.SPIFFTX.bit.TXFFST < 16) && (sent_count < length))
        {
            SpiaRegs.SPITXBUF = send_buffer[sent_count];
            sent_count++;
        }

        while (SpiaRegs.SPIFFTX.bit.TXFFST > 0) {}  // Block waiting for TX completion


        // Read RX FIFO
        while ((SpiaRegs.SPIFFRX.bit.RXFFST > 0) && (recv_count < length))
        {
            receive_buffer[recv_count] = SpiaRegs.SPIRXBUF;
            recv_count++;
        }
    }

    // Disable TX/RX FIFO
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 0;

    // Reset TX FIFO
    // SpiaRegs.SPIFFTX.bit.TXFIFO = 1;

    // Reset RX FIFO to clear it
    SpiaRegs.SPIFFRX.bit.RXFIFORESET = 0;
    SpiaRegs.SPIFFRX.bit.RXFFOVFCLR = 1;
}

void spi_send_bulk(Uint16* buffer, Uint16 *receive_buffer, Uint16 length)
{
    Uint16 sent_count = 0;
    Uint16 recv_count = 0;
    SpiaRegs.SPIFFTX.bit.SPIFFENA = 1;
    while (sent_count < length)
    {
        // Fill FIFO, max 16 data words
        while (SpiaRegs.SPIFFTX.bit.TXFFST < 16 && sent_count < length)
        {
            SpiaRegs.SPITXBUF = buffer[sent_count];
            sent_count++;
        }
        while (SpiaRegs.SPIFFTX.bit.TXFFST > 0) {}
    }

    // Read RX FIFO (as long as there is data and not fully received)
    while ((SpiaRegs.SPIFFRX.bit.RXFFST > 0))
    {
        receive_buffer[recv_count] = SpiaRegs.SPIRXBUF;
        recv_count++;
        if(recv_count >= length){
            recv_count = 0;
            printf("codec.c 1102 recv_count >= length");
        }
    }

    SpiaRegs.SPIFFTX.bit.SPIFFENA = 0;
}

/**
 * @brief SPI Loopback Test Function
 * This function sends data through SPI and receives it back to verify the integrity of the transmission.
 * @param send_buffer Pointer to the buffer containing data to be sent.
 * @param receive_buffer Pointer to the buffer where received data will be stored.
 * @param length Number of data words to send and receive.
 * @return int16_t Returns 0 if the test passes (data matches), -1 if there is a mismatch.
 */
int16 spi_lookback_test(Uint16 *send_buffer, Uint16 *receive_buffer, Uint16 length)
{
    spi_send_and_receive((Uint16*)send_buffer, (Uint16*)receive_buffer, length);

    // Verify data
    for(Uint16 i = 0; i < length; i++)
    {
        if(send_buffer[i] != receive_buffer[i])
        {
            return -1; // Data mismatch
        }
    }
    return 0; // Success

}
