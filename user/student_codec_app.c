#include <stdint.h>
#include <string.h>

#include "DSP2833x_Device.h"
#include "DSP2833x_Examples.h"

#include "audio.h"
#include "codec_service.h"
#include "exint.h"
#include "leds.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"

typedef enum {
    APP_STATE_IDLE = 0,
    APP_STATE_RECORD,
    APP_STATE_ENCODE,
    APP_STATE_G711_READY,
    APP_STATE_RECEIVE_READY,
    APP_STATE_DECODE,
    APP_STATE_PLAY,
    APP_STATE_STREAM_LOOPBACK,
    APP_STATE_STREAM_SPI_TX,
    APP_STATE_STREAM_SPI_RX_PLAY,
    APP_STATE_FLOOR_REQUEST,
    APP_STATE_FLOOR_WAIT_RESULT
} app_state_t;

static volatile app_state_t current_state = APP_STATE_IDLE;
static volatile Uint16 key_encode_pressed_flag = 0;
static volatile Uint32 key_encode_press_time = 0;
static volatile Uint16 key_decode_pressed_flag = 0;
static volatile Uint32 key_decode_press_time = 0;
static volatile Uint16 spi_ready_flag = 0;
static volatile Uint16 stream_loopback_start_pending = 0;
static volatile Uint16 stream_loopback_stop_pending = 0;
static volatile Uint16 stream_spi_tx_start_pending = 0;
static volatile Uint16 stream_spi_tx_stop_pending = 0;
static volatile Uint16 stream_spi_rx_play_start_pending = 0;
static volatile Uint16 stream_spi_rx_play_stop_pending = 0;
static volatile Uint16 floor_request_start_pending = 0;
static volatile Uint32 tick_count = 0;
static volatile Uint16 mcbsp_word_phase = 0;
static volatile Uint16 play_sample_hold = 0;
static Uint16 stream_loopback_frame[CODEC_SERVICE_STREAM_FRAME_OCTETS];
static Uint32 stream_loopback_frames = 0;
static Uint16 stream_spi_tx_frame[CODEC_SERVICE_STREAM_FRAME_OCTETS];
static Uint16 stream_spi_tx_packet[CODEC_SERVICE_STREAM_FRAME_OCTETS + 4U];
static Uint16 stream_spi_rx_packet[CODEC_SERVICE_STREAM_FRAME_OCTETS + 4U];
static Uint16 floor_tx_packet[CODEC_SERVICE_STREAM_FRAME_OCTETS + 4U];
static Uint16 floor_rx_packet[CODEC_SERVICE_STREAM_FRAME_OCTETS + 4U];
static Uint16 stream_spi_rx_play_tx_packet[CODEC_SERVICE_STREAM_FRAME_OCTETS + 4U];
static Uint16 stream_spi_rx_play_frame[CODEC_SERVICE_STREAM_FRAME_OCTETS];
static Uint32 stream_spi_rx_play_last_seq = 0;
static Uint32 stream_spi_rx_play_frames = 0;
static Uint32 stream_spi_rx_play_bad_magic = 0;
static Uint32 stream_spi_rx_play_bad_len = 0;
static Uint32 stream_spi_rx_play_gap = 0;
static Uint32 stream_spi_rx_play_fail_count = 0;
static Uint32 stream_spi_rx_play_last_ready_wait_us = 0;
static Uint32 stream_spi_rx_play_last_spi_us = 0;
static Uint16 stream_spi_rx_play_have_seq = 0;
static Uint16 stream_spi_rx_play_req_active = 0;
static Uint32 stream_spi_rx_play_req_start_us = 0;
static Uint32 stream_spi_tx_seq = 0;
static Uint32 stream_spi_tx_sent_count = 0;
static Uint32 stream_spi_tx_fail_count = 0;
static Uint32 stream_spi_tx_data_ready_active_count = 0;
static Uint32 stream_spi_tx_req_start_us = 0;
static Uint32 stream_spi_tx_last_send_us = 0;
static Uint32 stream_spi_tx_last_ready_wait_us = 0;
static Uint32 stream_spi_tx_last_spi_us = 0;
static Uint32 stream_spi_tx_last_interval_us = 0;
/* SPI 发送侧请求状态：1 表示已向从机发起发送请求，正在等待从机 SPI ready。 */
static Uint16 stream_spi_tx_req_active = 0;
static Uint32 floor_request_seq = 0;
static Uint32 floor_last_request_seq = 0;
static Uint32 floor_grant_count = 0;
static Uint32 floor_deny_count = 0;
static Uint32 floor_timeout_count = 0;
static Uint32 floor_bad_ctrl_count = 0;
static Uint32 floor_last_ready_wait_us = 0;
static Uint32 floor_last_spi_us = 0;
static Uint32 floor_wait_start_us = 0;
static Uint32 floor_req_start_us = 0;
static Uint16 floor_req_active = 0;

#define KEY_DEBOUNCE_MS 260U
#define APP_MONO_RECORD_WORD_SELECT 0U
#define APP_STREAM_LOOPBACK_LOG_FRAMES 50UL
#define APP_STREAM_LOOPBACK_PERIODIC_LOG 0U
#define APP_IDLE_TEST_MODE_LOOPBACK 0U
#define APP_IDLE_TEST_MODE_SPI_TX   1U
#define APP_IDLE_TEST_MODE_SPI_RX_PLAY 2U
#define APP_IDLE_TEST_MODE_FLOOR_PTT 3U
#define APP_IDLE_TEST_MODE          APP_IDLE_TEST_MODE_FLOOR_PTT
#define APP_STREAM_SPI_MAGIC        0x4711U
#define APP_FLOOR_SPI_MAGIC         0xF100U
#define APP_FLOOR_TYPE_REQUEST      1U
#define APP_FLOOR_TYPE_GRANT        2U
#define APP_FLOOR_TYPE_DENY         3U
#define APP_FLOOR_TYPE_TIMEOUT      4U
/* SPI 帧按 16 位 word 数组传输，固定下标定义头部，避免 DSP/MCU 结构体对齐和大小端差异。 */
#define APP_STREAM_SPI_HEADER_WORDS  4U
#define APP_STREAM_SPI_PACKET_WORDS  (APP_STREAM_SPI_HEADER_WORDS + \
                                      CODEC_SERVICE_STREAM_FRAME_OCTETS)
#define APP_STREAM_SPI_READY_TIMEOUT_US 20000UL
#define APP_FLOOR_TIMEOUT_US        1000000UL

static void init_zone7(void);
static Uint32 app_get_tick_count(void);
static void app_us_timer_init(void);
static Uint32 app_get_us(void);
static Uint32 app_elapsed_us(Uint32 start_us, Uint32 end_us);
static Uint16 app_slave_spi_ready_is_active(void);
static Uint16 app_slave_data_ready_is_active(void);
static void app_master_data_req_set(Uint16 active);
static void app_stream_loopback_start(void);
static void app_stream_loopback_stop(void);
static void app_stream_loopback_drain_pipeline(void);
static void app_stream_loopback_print_status(void);
static void app_stream_spi_tx_start(void);
static void app_stream_spi_tx_stop(void);
static void app_stream_spi_tx_service(void);
static void app_stream_spi_tx_print_status(void);
static void app_stream_spi_rx_play_start(void);
static void app_stream_spi_rx_play_stop(void);
static void app_stream_spi_rx_play_service(void);
static void app_stream_spi_rx_play_print_status(void);
static void app_downlink_voice_reset(Uint16 reset_codec);
static void app_downlink_voice_service(void);
static Uint16 app_downlink_voice_packet_is_idle(const Uint16 *packet);
static void app_downlink_voice_handle_packet(void);
static void app_floor_request_start(void);
static void app_floor_request_service(void);
static void app_floor_wait_result_service(void);
static void app_floor_abort_to_idle(char *message);
static void app_floor_print_status(void);
static void app_floor_clear_packet(Uint16 *packet);
static void app_floor_build_packet(Uint16 *packet, Uint16 type, Uint32 seq);
static Uint16 app_floor_check_timeout(Uint32 start_us);
static void delay(void);
void Delay(int16_t time);

interrupt void EXINT1_IRQn(void);
interrupt void EXINT2_IRQn(void);
interrupt void TIM0_IRQn(void);
interrupt void SPI_READY_IRQn(void);
interrupt void ISRMcbspSend(void);

int16_t main(int16_t argc, char **argv)
{
    int16_t result;

    InitSysCtrl();
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
    InitFlash();

    LED_Init();
    UARTa_Init(9600);

    EXINT1_Init(EXINT1_IRQn);
    EXINT2_Init(EXINT2_IRQn);

    InitSpiaGpio();
    spi_xon_init(); //流控引脚初始化
    // spi_ready_init(SPI_READY_IRQn);
    spi_init();

    InitMcbspaGpio();
    InitI2CGpio();
    I2CA_Init();
    AIC23Init();
    InitMcbspa();

    EALLOW;
    PieVectTable.MRINTA = &ISRMcbspSend;
    SysCtrlRegs.PCLKCR0.bit.SPIAENCLK = 1;
    EDIS;

    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
    PieCtrlRegs.PIEIER6.bit.INTx5 = 1;
    IER |= M_INT6;

    TIM0_Init(150, 10000, TIM0_IRQn);
    app_us_timer_init();

    EINT;
    ERTM;

    init_zone7();
    audio_set_tick_getter(app_get_tick_count);
    codec_service_reset();
    app_downlink_voice_reset(1U);

    UARTa_SendString("AIC32 codec SPI own app ready.\r\n");
    // 
    while (1) {
        if (stream_loopback_stop_pending != 0U) {
            stream_loopback_stop_pending = 0U;
            app_stream_loopback_stop();
        }

        if (stream_spi_tx_stop_pending != 0U) {
            stream_spi_tx_stop_pending = 0U;
            app_stream_spi_tx_stop();
        }

        if (stream_spi_rx_play_stop_pending != 0U) {
            stream_spi_rx_play_stop_pending = 0U;
            app_stream_spi_rx_play_stop();
        }

        // 初始化以及填充话权申请包
        if (floor_request_start_pending != 0U) {
            floor_request_start_pending = 0U;
            app_floor_request_start();
        }

        if (stream_loopback_start_pending != 0U) {
            stream_loopback_start_pending = 0U;
            app_stream_loopback_start();
        }

        if (stream_spi_tx_start_pending != 0U) {
            stream_spi_tx_start_pending = 0U;
            app_stream_spi_tx_start();
        }

        if (stream_spi_rx_play_start_pending != 0U) {
            stream_spi_rx_play_start_pending = 0U;
            app_stream_spi_rx_play_start();
        }

        if (current_state == APP_STATE_STREAM_LOOPBACK) {
            app_stream_loopback_drain_pipeline();
            continue;
        }

        if (current_state == APP_STATE_FLOOR_REQUEST) {
            app_floor_request_service();
            continue;
        }

        // 如果在这段时间按下按键会怎样
        // 会取消申请并清除相关的状态
        if (current_state == APP_STATE_FLOOR_WAIT_RESULT) {
            app_floor_wait_result_service();
            continue;
        }

        if (current_state == APP_STATE_STREAM_SPI_TX) {
            app_stream_spi_tx_service();
            continue;
        }

        if (current_state == APP_STATE_STREAM_SPI_RX_PLAY) {
            app_stream_spi_rx_play_service();
            continue;
        }

        if (current_state == APP_STATE_IDLE) {
            app_downlink_voice_service();
            continue;
        }

        if (current_state == APP_STATE_ENCODE) {
            Uint32 encode_start_tick;
            Uint32 encode_elapsed_ms;

            UARTa_SendString("### Starting encoding ###\r\n");
            UARTa_SendStringAndNumber("Recorded samples: ", codec_service_get_record_count(), "\r\n");
            // STEP 1
            encode_start_tick = tick_count;
            result = codec_service_encode_recorded();
            encode_elapsed_ms = (tick_count - encode_start_tick) * 10UL;
            UARTa_SendStringAndNumber("Encode time(ms): ", encode_elapsed_ms, "\r\n");
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendStringAndNumber("Encoding successful, G711 length: ", codec_service_get_g711_len(), "\r\n");
                current_state = APP_STATE_G711_READY;
            } else {
                UARTa_SendStringAndNumber("Encoding failed: ", result, "\r\n");
                current_state = APP_STATE_IDLE;
            }
        } else if (current_state == APP_STATE_G711_READY) {
            // 防止被误触发进入下一步，导致状态机混乱
            spi_ready_flag = 0;
            UARTa_SendString("### Starting SPI exchange one ###\r\n");
            // STEP 2
            result = codec_service_spi_exchange_first();
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendString("SPI exchange one successful.\r\n");
                current_state = APP_STATE_RECEIVE_READY;
            } else {
                UARTa_SendStringAndNumber("SPI exchange one failed: ", result, "\r\n");
                current_state = APP_STATE_IDLE;
            }
        } else if (current_state == APP_STATE_RECEIVE_READY) {
            UARTa_SendHex(spi_ready_flag);
            UARTa_SendString("\r\n");
            while (!spi_ready_flag) {
                UARTa_SendString("waiting......\r\n");
            }
            spi_ready_flag = 0;

            UARTa_SendString("### Starting SPI exchange two ###\r\n");
            // STEP 3
            result = codec_service_spi_exchange_second();
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendStringAndNumber("SPI exchange two successful, G711 length: ", codec_service_get_received_g711_len(), "\r\n");
                current_state = APP_STATE_DECODE;
            } else {
                const Uint16 *spi_rx_words = (const Uint16 *)codec_service_get_spi_rx_buffer();
                Uint16 debug_i;

                UARTa_SendStringAndNumber("SPI exchange two failed: ", result, "\r\n");
                UARTa_SendStringAndNumber("DSP expected G711 length: ", codec_service_get_g711_len(), "\r\n");
                UARTa_SendStringAndNumber("DSP parsed RX length: ", codec_service_get_received_g711_len(), "\r\n");
                UARTa_SendStringAndHex("DSP raw rx[0]: 0x", spi_rx_words[0], "\r\n");
                UARTa_SendString("DSP raw rx[0..7]: ");
                for (debug_i = 0; debug_i < 8U; debug_i++) {
                    UARTa_SendHex(spi_rx_words[debug_i]);
                    UARTa_SendString(" ");
                }
                UARTa_SendString("\r\n");
                current_state = APP_STATE_IDLE;
            }
        } else if (current_state == APP_STATE_DECODE) {
            Uint32 decode_start_tick;
            Uint32 decode_elapsed_ms;

            UARTa_SendString("### Starting decoding ###\r\n");
            // STEP 4
            decode_start_tick = tick_count;
            result = codec_service_decode_received();
            decode_elapsed_ms = (tick_count - decode_start_tick) * 10UL;
            UARTa_SendStringAndNumber("Decode time(ms): ", decode_elapsed_ms, "\r\n");
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendStringAndNumber("Decoding successful, PCM samples: ", codec_service_get_pcm_sample_count(), "\r\n");
                mcbsp_word_phase = 0;
                play_sample_hold = 0;
                current_state = APP_STATE_PLAY;
            } else {
                UARTa_SendStringAndNumber("Decoding failed: ", result, "\r\n");
                current_state = APP_STATE_IDLE;
            }
        }
    }
}

interrupt void EXINT1_IRQn(void)
{
    if (!key_encode_pressed_flag) {
        key_encode_pressed_flag = 1;
        key_encode_press_time = tick_count;
    }

    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
}

interrupt void EXINT2_IRQn(void)
{
    if (!key_decode_pressed_flag) {
        key_decode_pressed_flag = 1;
        key_decode_press_time = tick_count;
    }

    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
}

interrupt void TIM0_IRQn(void)
{
    tick_count++;

    if (key_encode_pressed_flag && (tick_count - key_encode_press_time >= KEY_DEBOUNCE_MS / 10U)) {
        if (current_state == APP_STATE_IDLE) {
            codec_service_start_record();
            mcbsp_word_phase = 0;
            play_sample_hold = 0;
            current_state = APP_STATE_RECORD;
            UARTa_SendString("Record start.\r\n");
        } else if (current_state == APP_STATE_RECORD) {
            mcbsp_word_phase = 0;
            current_state = APP_STATE_ENCODE;
            UARTa_SendString("Record stop.\r\n");
        }

        key_encode_pressed_flag = 0;
        LED4_TOGGLE;
    }

    if (key_decode_pressed_flag && (tick_count - key_decode_press_time >= KEY_DEBOUNCE_MS / 10U)) {
        if (current_state == APP_STATE_RECEIVE_READY) {
            current_state = APP_STATE_DECODE;
        } else if (current_state == APP_STATE_IDLE) {
#if APP_IDLE_TEST_MODE == APP_IDLE_TEST_MODE_SPI_TX
            stream_spi_tx_start_pending = 1U;
#elif APP_IDLE_TEST_MODE == APP_IDLE_TEST_MODE_SPI_RX_PLAY
            stream_spi_rx_play_start_pending = 1U;
#elif APP_IDLE_TEST_MODE == APP_IDLE_TEST_MODE_FLOOR_PTT
            /* 空闲状态下按功能键才会发起话权申请；此时主循环会暂停普通下行拉流，转入话权握手。 */
            floor_request_start_pending = 1U;
#else
            stream_loopback_start_pending = 1U;
#endif
        } else if (current_state == APP_STATE_STREAM_LOOPBACK) {
            stream_loopback_stop_pending = 1U;
        } else if (current_state == APP_STATE_STREAM_SPI_TX) {
            stream_spi_tx_stop_pending = 1U;
        } else if (current_state == APP_STATE_STREAM_SPI_RX_PLAY) {
            /* 显式接收播放状态下按功能键只停止接收播放，不会直接发起话权申请。 */
            stream_spi_rx_play_stop_pending = 1U;
        } else if ((current_state == APP_STATE_FLOOR_REQUEST) ||
                   (current_state == APP_STATE_FLOOR_WAIT_RESULT)) {
            app_master_data_req_set(0U);
            floor_req_active = 0U;
            current_state = APP_STATE_IDLE;
            UARTa_SendString("Floor request cancelled.\r\n");
            app_floor_print_status();
        }

        key_decode_pressed_flag = 0;
        LED3_TOGGLE;
    }

    static Uint16 temp_count = 0;
    if(++temp_count >= 50){
        temp_count = 0;
        LED1_TOGGLE;
    }
    
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
    CpuTimer0Regs.TCR.bit.TIF = 1;
    CpuTimer0Regs.TCR.bit.TRB = 1;
}

interrupt void SPI_READY_IRQn(void)
{
    spi_ready_flag = 1;
    PieCtrlRegs.PIEACK.bit.ACK12 = 1;
}

interrupt void ISRMcbspSend(void)
{
    Uint16 sample;
    Uint16 word_phase;
    int16_t temp;

    // 翻转相位状态，只有在特定相位时才进行读操作，以实现单声道录音
    word_phase = mcbsp_word_phase;
    mcbsp_word_phase ^= 1U;

    temp = McbspaRegs.DRR1.all;
    if (current_state == APP_STATE_RECORD) {
        if (word_phase == APP_MONO_RECORD_WORD_SELECT) {
            codec_service_record_sample(temp);
        }
        McbspaRegs.DXR1.all = temp;
    } else if (current_state == APP_STATE_STREAM_LOOPBACK) {
        if (word_phase == APP_MONO_RECORD_WORD_SELECT) {
            codec_service_stream_record_sample(temp);
            if (codec_service_stream_get_play_sample(&sample) ==
                    CODEC_SERVICE_OK) {
                play_sample_hold = sample;
            } else {
                play_sample_hold = 0;
            }
        }
        McbspaRegs.DXR1.all = play_sample_hold;
    } else if (current_state == APP_STATE_STREAM_SPI_TX) {
        if (word_phase == APP_MONO_RECORD_WORD_SELECT) {
            codec_service_stream_record_sample(temp);
        }
        McbspaRegs.DXR1.all = temp;
    } else if ((current_state == APP_STATE_STREAM_SPI_RX_PLAY) ||
               (current_state == APP_STATE_IDLE)) {
        if (word_phase == 0U) {
            if (codec_service_stream_get_play_sample(&sample) ==
                    CODEC_SERVICE_OK) {
                play_sample_hold = sample;
            } else {
                play_sample_hold = 0;
            }
        }
        McbspaRegs.DXR1.all = play_sample_hold;
    } else if ((current_state == APP_STATE_FLOOR_REQUEST) ||
               (current_state == APP_STATE_FLOOR_WAIT_RESULT)) {
        /* 话权申请握手期间不播放下行语音，先输出静音，避免语音包和控制包共用 SPI 时互相抢占。 */
        McbspaRegs.DXR1.all = 0U;
    } else if (current_state == APP_STATE_PLAY) {
        // 将单声道数据复制到左右声道输出，以实现单声道播放
        // 在相位0读一个样本，在相位1输出同一个样本
        if (word_phase == 0U) {
            if (codec_service_get_play_sample(&sample) == CODEC_SERVICE_PLAY_DONE) {
                current_state = APP_STATE_IDLE;
                play_sample_hold = 0;
                mcbsp_word_phase = 0;
            } else {
                play_sample_hold = sample;
            }
        }
        McbspaRegs.DXR1.all = play_sample_hold;
    }

    PieCtrlRegs.PIEACK.all = 0x0020;
}

static void init_zone7(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.XINTFENCLK = 1;
    EDIS;

    InitXintf16Gpio();

    EALLOW;
    XintfRegs.XINTCNF2.bit.XTIMCLK = 0;
    XintfRegs.XINTCNF2.bit.WRBUFF = 3;
    XintfRegs.XINTCNF2.bit.CLKOFF = 0;
    XintfRegs.XINTCNF2.bit.CLKMODE = 0;

    XintfRegs.XTIMING7.bit.XWRLEAD = 1;
    XintfRegs.XTIMING7.bit.XWRACTIVE = 2;
    XintfRegs.XTIMING7.bit.XWRTRAIL = 1;
    XintfRegs.XTIMING7.bit.XRDLEAD = 1;
    XintfRegs.XTIMING7.bit.XRDACTIVE = 3;
    XintfRegs.XTIMING7.bit.XRDTRAIL = 0;
    XintfRegs.XTIMING7.bit.X2TIMING = 0;
    XintfRegs.XTIMING7.bit.USEREADY = 0;
    XintfRegs.XTIMING7.bit.READYMODE = 0;
    XintfRegs.XTIMING7.bit.XSIZE = 3;
    EDIS;

    asm(" RPT #7 || NOP");
}

static Uint32 app_get_tick_count(void)
{
    return tick_count;
}

static void app_us_timer_init(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR3.bit.CPUTIMER1ENCLK = 1;
    EDIS;

    CpuTimer1Regs.PRD.all = 0xFFFFFFFFUL;
    CpuTimer1Regs.TPR.all = 149U;
    CpuTimer1Regs.TPRH.all = 0U;
    CpuTimer1Regs.TCR.bit.TSS = 1U;
    CpuTimer1Regs.TCR.bit.TRB = 1U;
    CpuTimer1Regs.TCR.bit.TIE = 0U;
    CpuTimer1Regs.TCR.bit.FREE = 1U;
    CpuTimer1Regs.TCR.bit.SOFT = 1U;
    CpuTimer1Regs.TCR.bit.TSS = 0U;
}

static Uint32 app_get_us(void)
{
    return 0xFFFFFFFFUL - CpuTimer1Regs.TIM.all;
}

static Uint32 app_elapsed_us(Uint32 start_us, Uint32 end_us)
{
    return end_us - start_us;
}

static Uint16 app_slave_spi_ready_is_active(void)
{
    return (GpioDataRegs.GPADAT.bit.GPIO10 == 0U) ? 1U : 0U;
}

static Uint16 app_slave_data_ready_is_active(void)
{
    return (GpioDataRegs.GPBDAT.bit.GPIO50 == 0U) ? 1U : 0U;
}

static void app_master_data_req_set(Uint16 active)
{
    if (active != 0U) {
        GpioDataRegs.GPACLEAR.bit.GPIO11 = 1U;
    } else {
        GpioDataRegs.GPASET.bit.GPIO11 = 1U;
    }
}

static void app_stream_loopback_start(void)
{
    codec_service_stream_reset();
    codec_service_stream_start_capture();
    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    stream_loopback_frames = 0;
    current_state = APP_STATE_STREAM_LOOPBACK;
    UARTa_SendString("Stream loopback start.\r\n");
}

static void app_stream_loopback_stop(void)
{
    codec_service_stream_stop_capture();
    current_state = APP_STATE_IDLE;
    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    UARTa_SendString("Stream loopback stop.\r\n");
    app_stream_loopback_print_status();
}

static void app_stream_spi_tx_start(void)
{
    codec_service_stream_reset();
    codec_service_stream_start_capture();
    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    stream_spi_tx_seq = 0;
    stream_spi_tx_sent_count = 0;
    stream_spi_tx_fail_count = 0;
    stream_spi_tx_data_ready_active_count = 0;
    stream_spi_tx_req_start_us = 0;
    stream_spi_tx_last_send_us = 0;
    stream_spi_tx_last_ready_wait_us = 0;
    stream_spi_tx_last_spi_us = 0;
    stream_spi_tx_last_interval_us = 0;
    stream_spi_tx_req_active = 0;
    app_master_data_req_set(0U);
    current_state = APP_STATE_STREAM_SPI_TX;
    UARTa_SendString("Stream SPI TX start.\r\n");
}

static void app_stream_spi_tx_stop(void)
{
    codec_service_stream_stop_capture();
    app_master_data_req_set(0U);
    stream_spi_tx_req_active = 0;
    /* TODO: Send FLOOR_RELEASE here when explicit floor release is added. */
    current_state = APP_STATE_IDLE;
    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    UARTa_SendString("Stream SPI TX stop.\r\n");
    app_stream_spi_tx_print_status();
    app_downlink_voice_reset(1U);
}

static void app_stream_spi_rx_play_start(void)
{
    app_downlink_voice_reset(1U);
    current_state = APP_STATE_STREAM_SPI_RX_PLAY;
    UARTa_SendString("Stream SPI RX play start.\r\n");
}

static void app_stream_spi_rx_play_stop(void)
{
    app_master_data_req_set(0U);
    stream_spi_rx_play_req_active = 0;
    current_state = APP_STATE_IDLE;
    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    UARTa_SendString("Stream SPI RX play stop.\r\n");
    app_stream_spi_rx_play_print_status();
}

static void app_downlink_voice_reset(Uint16 reset_codec)
{
    Uint16 i;

    if (reset_codec != 0U) {
        codec_service_stream_reset();
    }

    mcbsp_word_phase = 0;
    play_sample_hold = 0;
    stream_spi_rx_play_last_seq = 0;
    stream_spi_rx_play_frames = 0;
    stream_spi_rx_play_bad_magic = 0;
    stream_spi_rx_play_bad_len = 0;
    stream_spi_rx_play_gap = 0;
    stream_spi_rx_play_fail_count = 0;
    stream_spi_rx_play_last_ready_wait_us = 0;
    stream_spi_rx_play_last_spi_us = 0;
    stream_spi_rx_play_have_seq = 0;
    stream_spi_rx_play_req_active = 0;
    stream_spi_rx_play_req_start_us = 0;

    for (i = 0U; i < APP_STREAM_SPI_PACKET_WORDS; i++) {
        stream_spi_rx_play_tx_packet[i] = 0U;
        stream_spi_rx_packet[i] = 0U;
    }

    app_master_data_req_set(0U);
}

static void app_floor_clear_packet(Uint16 *packet)
{
    Uint16 i;

    for (i = 0U; i < APP_STREAM_SPI_PACKET_WORDS; i++) {
        packet[i] = 0U;
    }
}

static void app_floor_build_packet(Uint16 *packet, Uint16 type, Uint32 seq)
{
    app_floor_clear_packet(packet);
    packet[0] = APP_FLOOR_SPI_MAGIC;
    packet[1] = type;
    packet[2] = (Uint16)(seq & 0xFFFFUL);
    packet[3] = (Uint16)((seq >> 16) & 0xFFFFUL);
}

static Uint16 app_floor_check_timeout(Uint32 start_us)
{
    Uint32 now_us;

    now_us = app_get_us();
    return (app_elapsed_us(start_us, now_us) > APP_FLOOR_TIMEOUT_US) ?
            1U : 0U;
}

static void app_floor_abort_to_idle(char *message)
{
    app_master_data_req_set(0U);
    floor_req_active = 0U;
    current_state = APP_STATE_IDLE;
    UARTa_SendString(message);
    app_floor_print_status();
}

static void app_floor_request_start(void)
{
    floor_last_request_seq = floor_request_seq++;
    floor_req_active = 0U;
    floor_req_start_us = 0UL;
    floor_wait_start_us = 0UL;
    floor_last_ready_wait_us = 0UL;
    floor_last_spi_us = 0UL;
    app_master_data_req_set(0U);
    app_floor_build_packet(floor_tx_packet,
                           APP_FLOOR_TYPE_REQUEST,
                           floor_last_request_seq);
    app_floor_clear_packet(floor_rx_packet);
    current_state = APP_STATE_FLOOR_REQUEST;
    UARTa_SendStringAndNumber("Floor request start seq:",
                              (int32)floor_last_request_seq,
                              "\r\n");
}

static void app_floor_request_service(void)
{
    Uint32 now_us;
    Uint32 spi_start_us;
    Uint32 spi_end_us;
    Uint32 ready_wait_start_us;
    Uint32 ready_wait_now_us;

    // 只执行一次
    if (floor_req_active == 0U) {
        floor_req_active = 1U;
        floor_req_start_us = app_get_us();
        app_master_data_req_set(1U);
    }
    // 非阻塞等待
    if (app_slave_spi_ready_is_active() == 0U) {
        if (app_floor_check_timeout(floor_req_start_us) != 0U) {
            floor_timeout_count++;
            app_floor_abort_to_idle("Floor request timeout.\r\n");
        }
        return;
    }

    now_us = app_get_us();
    floor_last_ready_wait_us = app_elapsed_us(floor_req_start_us, now_us);
    app_master_data_req_set(0U);
    floor_req_active = 0U;

    spi_start_us = app_get_us();
    spi_send_and_receive(floor_tx_packet,
                         floor_rx_packet,
                         APP_STREAM_SPI_PACKET_WORDS);
    spi_end_us = app_get_us();
    floor_last_spi_us = app_elapsed_us(spi_start_us, spi_end_us);

    ready_wait_start_us = app_get_us();
    while (app_slave_spi_ready_is_active() != 0U) {
        ready_wait_now_us = app_get_us();
        if (app_elapsed_us(ready_wait_start_us,
                           ready_wait_now_us) > APP_STREAM_SPI_READY_TIMEOUT_US) {
            floor_timeout_count++;
            app_floor_abort_to_idle("Floor request ready release timeout.\r\n");
            return;
        }
    }

    floor_wait_start_us = app_get_us();
    current_state = APP_STATE_FLOOR_WAIT_RESULT;
    UARTa_SendString("Floor request sent, waiting result.\r\n");
}

static void app_floor_wait_result_service(void)
{
    Uint16 type;
    Uint32 seq;
    Uint32 now_us;
    Uint32 spi_start_us;
    Uint32 spi_end_us;
    Uint32 ready_wait_start_us;
    Uint32 ready_wait_now_us;

    // 等待MCU返回话权申请结果
    if (app_slave_data_ready_is_active() == 0U) {
        if (app_floor_check_timeout(floor_wait_start_us) != 0U) {
            floor_timeout_count++;
            app_floor_abort_to_idle("Floor result timeout.\r\n");
        }
        return;
    }

    if (floor_req_active == 0U) {
        floor_req_active = 1U;
        floor_req_start_us = app_get_us();
        app_master_data_req_set(1U);
    }

    if (app_slave_spi_ready_is_active() == 0U) {
        if (app_floor_check_timeout(floor_req_start_us) != 0U) {
            floor_timeout_count++;
            app_floor_abort_to_idle("Floor result SPI ready timeout.\r\n");
        }
        return;
    }

    now_us = app_get_us();
    floor_last_ready_wait_us = app_elapsed_us(floor_req_start_us, now_us);
    app_master_data_req_set(0U);
    floor_req_active = 0U;

    app_floor_clear_packet(floor_tx_packet);
    app_floor_clear_packet(floor_rx_packet);
    spi_start_us = app_get_us();
    spi_send_and_receive(floor_tx_packet,
                         floor_rx_packet,
                         APP_STREAM_SPI_PACKET_WORDS);
    spi_end_us = app_get_us();
    floor_last_spi_us = app_elapsed_us(spi_start_us, spi_end_us);

    ready_wait_start_us = app_get_us();
    // 等待从机释放 spi_ready，以此确认上一轮通信已经结束
    while (app_slave_spi_ready_is_active() != 0U) {
        ready_wait_now_us = app_get_us();
        if (app_elapsed_us(ready_wait_start_us,
                           ready_wait_now_us) > APP_STREAM_SPI_READY_TIMEOUT_US) {
            floor_timeout_count++;
            app_floor_abort_to_idle("Floor result ready release timeout.\r\n");
            return;
        }
    }

    if (floor_rx_packet[0] != APP_FLOOR_SPI_MAGIC) {
        floor_bad_ctrl_count++;
        app_floor_abort_to_idle("Floor bad control magic.\r\n");
        return;
    }

    type = floor_rx_packet[1];
    seq = ((Uint32)floor_rx_packet[3] << 16) |
          (Uint32)floor_rx_packet[2];
    if (seq != floor_last_request_seq) {
        floor_bad_ctrl_count++;
        app_floor_abort_to_idle("Floor bad control seq.\r\n");
        return;
    }

    if (type == APP_FLOOR_TYPE_GRANT) {
        floor_grant_count++;
        UARTa_SendString("Floor grant.\r\n");
        app_floor_print_status();
        // 初始化并置位APP_STATE_STREAM_SPI_TX状态
        app_stream_spi_tx_start();
    } else if (type == APP_FLOOR_TYPE_DENY) {
        floor_deny_count++;
        app_floor_abort_to_idle("Floor deny.\r\n");
    } else {
        floor_timeout_count++;
        app_floor_abort_to_idle("Floor server timeout/error.\r\n");
    }
}

static void app_stream_spi_tx_service(void)
{
    Uint16 out_words;
    Uint16 i;
    Uint32 now_us;
    Uint32 spi_start_us;
    Uint32 spi_end_us;
    Uint32 ready_wait_start_us;
    Uint32 ready_wait_now_us;

    // PCM队列
    while (codec_service_stream_has_pcm_frame() != 0U) {
        if (codec_service_stream_process_encode() != CODEC_SERVICE_OK) {
            break;
        }
    }

    // 编码队列
    if (codec_service_stream_has_encoded_frame() == 0U) {
        app_master_data_req_set(0U);
        /* 没有待发送编码帧时，撤销主机请求并清除等待从机 ready 的状态。 */
        stream_spi_tx_req_active = 0;
        return;
    }

    /* 正常半双工 TX 期间一般不应收到下行 data_ready；若出现则只做诊断计数，不影响本次发送。 */
    if (app_slave_data_ready_is_active() != 0U) {
        stream_spi_tx_data_ready_active_count++;
    }

    // 防止在等待从机spi_ready信号期间重复设置req信号
    if (stream_spi_tx_req_active == 0U) {
        /* 本轮首次发现有编码帧可发，拉起请求信号，并记录等待起点。 */
        stream_spi_tx_req_active = 1U;
        stream_spi_tx_req_start_us = app_get_us();
        app_master_data_req_set(1U);
    }

    // 非阻塞等待
    if (app_slave_spi_ready_is_active() == 0U) {
        return;
    }

    now_us = app_get_us();
    stream_spi_tx_last_ready_wait_us =
            app_elapsed_us(stream_spi_tx_req_start_us, now_us);
    app_master_data_req_set(0U);
    /* 从机已 ready，可以开始 SPI 传输，本次请求状态结束。 */
    stream_spi_tx_req_active = 0;

    if (codec_service_stream_get_encoded_frame(stream_spi_tx_frame,
                                               CODEC_SERVICE_STREAM_FRAME_OCTETS,
                                               &out_words) != CODEC_SERVICE_OK) {
        return;
    }

    if (out_words != CODEC_SERVICE_STREAM_FRAME_OCTETS) {
        stream_spi_tx_fail_count++;
        return;
    }

    stream_spi_tx_packet[0] = APP_STREAM_SPI_MAGIC;
    stream_spi_tx_packet[1] = (Uint16)(stream_spi_tx_seq & 0xFFFFUL);
    stream_spi_tx_packet[2] = (Uint16)((stream_spi_tx_seq >> 16) & 0xFFFFUL);
    stream_spi_tx_packet[3] = CODEC_SERVICE_STREAM_FRAME_OCTETS;
    for (i = 0U; i < CODEC_SERVICE_STREAM_FRAME_OCTETS; i++) {
        stream_spi_tx_packet[i + APP_STREAM_SPI_HEADER_WORDS] =
                stream_spi_tx_frame[i] & 0x00FFU;
    }

    spi_start_us = app_get_us();
    if (stream_spi_tx_last_send_us != 0UL) {
        stream_spi_tx_last_interval_us =
                app_elapsed_us(stream_spi_tx_last_send_us, spi_start_us);
    }

    spi_send_and_receive(stream_spi_tx_packet,
                         stream_spi_rx_packet,
                         APP_STREAM_SPI_PACKET_WORDS);
    spi_end_us = app_get_us();
    stream_spi_tx_last_spi_us = app_elapsed_us(spi_start_us, spi_end_us);
    stream_spi_tx_last_send_us = spi_start_us;

    ready_wait_start_us = app_get_us();
    while (app_slave_spi_ready_is_active() != 0U) {
        ready_wait_now_us = app_get_us();
        if (app_elapsed_us(ready_wait_start_us,
                           ready_wait_now_us) > APP_STREAM_SPI_READY_TIMEOUT_US) {
            stream_spi_tx_fail_count++;
            break;
        }
    }

    stream_spi_tx_seq++;
    stream_spi_tx_sent_count++;
}

static void app_stream_spi_rx_play_service(void)
{
    app_downlink_voice_service();
}

static void app_downlink_voice_service(void)
{
    Uint32 now_us;
    Uint32 spi_start_us;
    Uint32 spi_end_us;
    Uint32 ready_wait_start_us;
    Uint32 ready_wait_now_us;

    /* 播放缓冲区已满时，停止向从机请求新的下行语音数据，避免溢出。 */
    if (codec_service_stream_get_play_frame_count() >=
            CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY) {
        app_master_data_req_set(0U);
        stream_spi_rx_play_req_active = 0;
        return;
    }

    /* 从机没有准备好下行数据时，清除本次请求状态并退出。 */
    if (app_slave_data_ready_is_active() == 0U) {
        app_master_data_req_set(0U);
        stream_spi_rx_play_req_active = 0;
        return;
    }

    /* 首次发现从机有数据时，拉高主机请求信号，并记录开始等待的时间。 */
    if (stream_spi_rx_play_req_active == 0U) {
        stream_spi_rx_play_req_active = 1U;
        stream_spi_rx_play_req_start_us = app_get_us();
        app_master_data_req_set(1U);
    }

    /* 等待从机拉起 SPI ready；未就绪时本轮不阻塞，等下次调度再检查。 */
    if (app_slave_spi_ready_is_active() == 0U) {
        return;
    }

    /* 从机已就绪，统计从发起请求到 ready 的等待时间，并撤销请求信号。 */
    now_us = app_get_us();
    stream_spi_rx_play_last_ready_wait_us =
            app_elapsed_us(stream_spi_rx_play_req_start_us, now_us);
    app_master_data_req_set(0U);
    stream_spi_rx_play_req_active = 0;

    /* 通过 SPI 全双工收取一包下行语音数据，同时记录本次 SPI 传输耗时。 */
    spi_start_us = app_get_us();
    spi_send_and_receive(stream_spi_rx_play_tx_packet,
                         stream_spi_rx_packet,
                         APP_STREAM_SPI_PACKET_WORDS);
    spi_end_us = app_get_us();
    stream_spi_rx_play_last_spi_us = app_elapsed_us(spi_start_us, spi_end_us);

    /* 等待从机释放 SPI ready，确保下一轮不会误把上一轮 ready 高电平当成新一次就绪。 */
    ready_wait_start_us = app_get_us();
    while (app_slave_spi_ready_is_active() != 0U) {
        ready_wait_now_us = app_get_us();
        if (app_elapsed_us(ready_wait_start_us,
                           ready_wait_now_us) > APP_STREAM_SPI_READY_TIMEOUT_US) {
            stream_spi_rx_play_fail_count++;
            break;
        }
    }

    /* 解析刚收到的 SPI 包，并把有效语音帧送入播放流。 */
    app_downlink_voice_handle_packet();
}

static Uint16 app_downlink_voice_packet_is_idle(const Uint16 *packet)
{
    Uint16 i;

    for (i = 0U; i < APP_STREAM_SPI_PACKET_WORDS; i++) {
        if (packet[i] != 0U) {
            return 0U;
        }
    }

    return 1U;
}

static void app_downlink_voice_handle_packet(void)
{
    Uint16 i;
    Uint16 magic;
    Uint16 payload_words;
    Uint32 seq;

    if (app_downlink_voice_packet_is_idle(stream_spi_rx_packet) != 0U) {
        return;
    }

    magic = stream_spi_rx_packet[0];
    if (magic == APP_FLOOR_SPI_MAGIC) {
        return;
    }

    if (magic != APP_STREAM_SPI_MAGIC) {
        stream_spi_rx_play_bad_magic++;
        return;
    }

    payload_words = stream_spi_rx_packet[3];
    if (payload_words != CODEC_SERVICE_STREAM_FRAME_OCTETS) {
        stream_spi_rx_play_bad_len++;
        return;
    }

    seq = ((Uint32)stream_spi_rx_packet[2] << 16) |
          (Uint32)stream_spi_rx_packet[1];
    if ((stream_spi_rx_play_have_seq != 0U) &&
        (seq != (stream_spi_rx_play_last_seq + 1UL))) {
        stream_spi_rx_play_gap++;
    }

    for (i = 0U; i < CODEC_SERVICE_STREAM_FRAME_OCTETS; i++) {
        stream_spi_rx_play_frame[i] =
                stream_spi_rx_packet[i + APP_STREAM_SPI_HEADER_WORDS] & 0x00FFU;
    }

    if (codec_service_stream_put_play_frame(stream_spi_rx_play_frame,
                                            CODEC_SERVICE_STREAM_FRAME_OCTETS) !=
            CODEC_SERVICE_OK) {
        stream_spi_rx_play_fail_count++;
        return;
    }

    stream_spi_rx_play_last_seq = seq;
    stream_spi_rx_play_have_seq = 1U;
    stream_spi_rx_play_frames++;
}

static void app_stream_loopback_drain_pipeline(void)
{
    Uint16 out_words;

    while (codec_service_stream_has_pcm_frame() != 0U) {
        if (codec_service_stream_process_encode() != CODEC_SERVICE_OK) {
            break;
        }
    }

    while ((codec_service_stream_has_encoded_frame() != 0U) &&
           (codec_service_stream_get_play_frame_count() <
            CODEC_SERVICE_STREAM_PLAY_FRAME_CAPACITY)) {
        if (codec_service_stream_get_encoded_frame(stream_loopback_frame,
                                                  CODEC_SERVICE_STREAM_FRAME_OCTETS,
                                                  &out_words) != CODEC_SERVICE_OK) {
            break;
        }

        if (out_words != CODEC_SERVICE_STREAM_FRAME_OCTETS) {
            break;
        }

        if (codec_service_stream_put_play_frame(stream_loopback_frame,
                                                out_words) != CODEC_SERVICE_OK) {
            break;
        }

        stream_loopback_frames++;
#if APP_STREAM_LOOPBACK_PERIODIC_LOG
        if ((stream_loopback_frames % APP_STREAM_LOOPBACK_LOG_FRAMES) == 0UL) {
            app_stream_loopback_print_status();
        }
#endif
    }
}

static void app_stream_loopback_print_status(void)
{
    UARTa_SendStringAndNumber("S pcm:",
                              codec_service_stream_get_pcm_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("enc:",
                              codec_service_stream_get_encoded_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("play:",
                              codec_service_stream_get_play_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("ov:",
                              codec_service_stream_get_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf:",
                              codec_service_stream_get_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("ov_pcm:",
                              codec_service_stream_get_pcm_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("ov_enc:",
                              codec_service_stream_get_encoded_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("ov_play:",
                              codec_service_stream_get_play_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf_pcm:",
                              codec_service_stream_get_pcm_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf_enc:",
                              codec_service_stream_get_encoded_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf_play:",
                              codec_service_stream_get_play_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("max_pcm:",
                              codec_service_stream_get_max_pcm_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("max_enc:",
                              codec_service_stream_get_max_encoded_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("max_play:",
                              codec_service_stream_get_max_play_frame_count(),
                              "\r\n");
}

static void app_stream_spi_tx_print_status(void)
{
    UARTa_SendStringAndNumber("TX seq:",
                              (int32)stream_spi_tx_seq,
                              " ");
    UARTa_SendStringAndNumber("pcm:",
                              codec_service_stream_get_pcm_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("enc:",
                              codec_service_stream_get_encoded_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("ov:",
                              codec_service_stream_get_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf:",
                              codec_service_stream_get_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("rw_us:",
                              (int32)stream_spi_tx_last_ready_wait_us,
                              " ");
    UARTa_SendStringAndNumber("spi_us:",
                              (int32)stream_spi_tx_last_spi_us,
                              " ");
    UARTa_SendStringAndNumber("int_us:",
                              (int32)stream_spi_tx_last_interval_us,
                              " ");
    UARTa_SendStringAndNumber("drdy_act:",
                              (int32)stream_spi_tx_data_ready_active_count,
                              " ");
    UARTa_SendStringAndNumber("fail:",
                              (int32)stream_spi_tx_fail_count,
                              "\r\n");
}

static void app_stream_spi_rx_play_print_status(void)
{
    UARTa_SendStringAndNumber("RX seq:",
                              (int32)stream_spi_rx_play_last_seq,
                              " ");
    UARTa_SendStringAndNumber("frames:",
                              (int32)stream_spi_rx_play_frames,
                              " ");
    UARTa_SendStringAndNumber("bad_magic:",
                              (int32)stream_spi_rx_play_bad_magic,
                              " ");
    UARTa_SendStringAndNumber("bad_len:",
                              (int32)stream_spi_rx_play_bad_len,
                              " ");
    UARTa_SendStringAndNumber("gap:",
                              (int32)stream_spi_rx_play_gap,
                              " ");
    UARTa_SendStringAndNumber("play:",
                              codec_service_stream_get_play_frame_count(),
                              " ");
    UARTa_SendStringAndNumber("ov:",
                              codec_service_stream_get_overflow_count(),
                              " ");
    UARTa_SendStringAndNumber("uf:",
                              codec_service_stream_get_underflow_count(),
                              " ");
    UARTa_SendStringAndNumber("rw_us:",
                              (int32)stream_spi_rx_play_last_ready_wait_us,
                              " ");
    UARTa_SendStringAndNumber("spi_us:",
                              (int32)stream_spi_rx_play_last_spi_us,
                              " ");
    UARTa_SendStringAndNumber("fail:",
                              (int32)stream_spi_rx_play_fail_count,
                              "\r\n");
}

static void app_floor_print_status(void)
{
    UARTa_SendStringAndNumber("FLOOR seq:",
                              (int32)floor_last_request_seq,
                              " ");
    UARTa_SendStringAndNumber("grant:",
                              (int32)floor_grant_count,
                              " ");
    UARTa_SendStringAndNumber("deny:",
                              (int32)floor_deny_count,
                              " ");
    UARTa_SendStringAndNumber("timeout:",
                              (int32)floor_timeout_count,
                              " ");
    UARTa_SendStringAndNumber("bad:",
                              (int32)floor_bad_ctrl_count,
                              " ");
    UARTa_SendStringAndNumber("rw_us:",
                              (int32)floor_last_ready_wait_us,
                              " ");
    UARTa_SendStringAndNumber("spi_us:",
                              (int32)floor_last_spi_us,
                              "\r\n");
}

static void delay(void)
{
    Uint16 i;
    Uint32 j;

    for (i = 0; i < 32; i++) {
        for (j = 0; j < 100000; j++) {
        }
    }
}

void Delay(int16_t time)
{
    int16_t i;
    int16_t j;
    volatile int16_t k = 0;

    for (i = 0; i < time; i++) {
        for (j = 0; j < 1024; j++) {
            k++;
        }
    }
}
