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
    APP_STATE_STREAM_LOOPBACK
} app_state_t;

static volatile app_state_t current_state = APP_STATE_IDLE;
static volatile Uint16 key_encode_pressed_flag = 0;
static volatile Uint32 key_encode_press_time = 0;
static volatile Uint16 key_decode_pressed_flag = 0;
static volatile Uint32 key_decode_press_time = 0;
static volatile Uint16 spi_ready_flag = 0;
static volatile Uint16 stream_loopback_start_pending = 0;
static volatile Uint16 stream_loopback_stop_pending = 0;
static volatile Uint32 tick_count = 0;
static volatile Uint16 mcbsp_word_phase = 0;
static volatile Uint16 play_sample_hold = 0;
static Uint16 stream_loopback_frame[CODEC_SERVICE_STREAM_FRAME_OCTETS];
static Uint32 stream_loopback_frames = 0;

#define KEY_DEBOUNCE_MS 260U
#define APP_MONO_RECORD_WORD_SELECT 0U
#define APP_STREAM_LOOPBACK_LOG_FRAMES 50UL
#define APP_STREAM_LOOPBACK_PERIODIC_LOG 0U

static void init_zone7(void);
static Uint32 app_get_tick_count(void);
static void app_stream_loopback_start(void);
static void app_stream_loopback_stop(void);
static void app_stream_loopback_drain_pipeline(void);
static void app_stream_loopback_print_status(void);
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
    spi_ready_init(SPI_READY_IRQn);
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

    EINT;
    ERTM;

    init_zone7();
    audio_set_tick_getter(app_get_tick_count);
    codec_service_reset();

    UARTa_SendString("AIC32 codec SPI own app ready.\r\n");

    while (1) {
        if (stream_loopback_stop_pending != 0U) {
            stream_loopback_stop_pending = 0U;
            app_stream_loopback_stop();
        }

        if (stream_loopback_start_pending != 0U) {
            stream_loopback_start_pending = 0U;
            app_stream_loopback_start();
        }

        if (current_state == APP_STATE_STREAM_LOOPBACK) {
            app_stream_loopback_drain_pipeline();
            continue;
        }

        // LED1_TOGGLE;
        // delay();

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
            stream_loopback_start_pending = 1U;
        } else if (current_state == APP_STATE_STREAM_LOOPBACK) {
            stream_loopback_stop_pending = 1U;
        }

        key_decode_pressed_flag = 0;
        LED3_TOGGLE;
    }

    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
    CpuTimer0Regs.TCR.bit.TIF = 1;
    CpuTimer0Regs.TCR.bit.TRB = 1;
}

interrupt void SPI_READY_IRQn(void)
{
    UARTa_SendString("nnnnnnnnnnnnnnnnn\r\n");
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
    } else if (current_state == APP_STATE_PLAY) {
        // 将单声道数据复制到左右声道输出，以实现单声道播放
        // 在相位0读一个样本，在相位1输出同一个样本
        if (word_phase == 0U) {
            if (codec_service_get_play_sample(&sample) == CODEC_SERVICE_PLAY_DONE) {
                UARTa_SendString("Play complete.\r\n");
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
