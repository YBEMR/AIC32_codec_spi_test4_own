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
    APP_STATE_AMR_READY,
    APP_STATE_RECEIVE_READY,
    APP_STATE_DECODE,
    APP_STATE_PLAY
} app_state_t;

static volatile app_state_t current_state = APP_STATE_IDLE;
static volatile Uint16 key_encode_pressed_flag = 0;
static volatile Uint32 key_encode_press_time = 0;
static volatile Uint16 key_decode_pressed_flag = 0;
static volatile Uint32 key_decode_press_time = 0;
static volatile Uint16 spi_ready_flag = 0;
static volatile Uint32 tick_count = 0;

#define KEY_DEBOUNCE_MS 260U

static void init_zone7(void);
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
    codec_service_reset();

    UARTa_SendString("AIC32 codec SPI student app ready.\r\n");

    while (1) {
        LED1_TOGGLE;
        delay();

        if (current_state == APP_STATE_ENCODE) {
            UARTa_SendString("### Starting encoding ###\r\n");
            UARTa_SendStringAndNumber("Recorded samples: ", codec_service_get_record_count(), "\r\n");
            // STEP 1
            result = codec_service_encode_recorded();
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendStringAndNumber("Encoding successful, AMR length: ", codec_service_get_amr_len(), "\r\n");
                current_state = APP_STATE_AMR_READY;
            } else {
                UARTa_SendStringAndNumber("Encoding failed: ", result, "\r\n");
                current_state = APP_STATE_IDLE;
            }
        } else if (current_state == APP_STATE_AMR_READY) {
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
                UARTa_SendStringAndNumber("SPI exchange two successful, AMR length: ", codec_service_get_received_amr_len(), "\r\n");
                current_state = APP_STATE_DECODE;
            } else {
                const Uint16 *spi_rx_words = (const Uint16 *)codec_service_get_spi_rx_buffer();
                Uint16 debug_i;

                UARTa_SendStringAndNumber("SPI exchange two failed: ", result, "\r\n");
                UARTa_SendStringAndNumber("DSP expected AMR length: ", codec_service_get_amr_len(), "\r\n");
                UARTa_SendStringAndNumber("DSP parsed RX length: ", codec_service_get_received_amr_len(), "\r\n");
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
            UARTa_SendString("### Starting decoding ###\r\n");
            // STEP 4
            result = codec_service_decode_received();
            if (result == CODEC_SERVICE_OK) {
                UARTa_SendStringAndNumber("Decoding successful, WAV length: ", codec_service_get_wav_len(), "\r\n");
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
            current_state = APP_STATE_RECORD;
            UARTa_SendString("Record start.\r\n");
        } else if (current_state == APP_STATE_RECORD) {
            current_state = APP_STATE_ENCODE;
            UARTa_SendString("Record stop.\r\n");
        }

        key_encode_pressed_flag = 0;
        LED4_TOGGLE;
    }

    if (key_decode_pressed_flag && (tick_count - key_decode_press_time >= KEY_DEBOUNCE_MS / 10U)) {
        if (current_state == APP_STATE_RECEIVE_READY) {
            current_state = APP_STATE_DECODE;
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
    int16_t temp;

    temp = McbspaRegs.DRR1.all;
    if (current_state == APP_STATE_RECORD) {
        codec_service_record_sample(temp);
        McbspaRegs.DXR1.all = temp;
    } else if (current_state == APP_STATE_PLAY) {
        if (codec_service_get_play_sample(&sample) == CODEC_SERVICE_PLAY_DONE) {
            UARTa_SendString("Play complete.\r\n");
            current_state = APP_STATE_IDLE;
            McbspaRegs.DXR1.all = 0;
        } else {
            McbspaRegs.DXR1.all = sample;
        }
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
