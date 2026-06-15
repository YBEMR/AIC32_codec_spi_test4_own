#include "amr_profile.h"

#if AMR_PROFILE

#include "DSP2833x_Device.h"

extern void UARTa_SendStringAndNumber(char *msg1, int32 number, char *msg2);

#define AMR_PROF_CPU_MHZ 150ULL

static unsigned long long amr_prof_cycles[AMR_PROF_STAGE_COUNT];
static amr_prof_tick_getter_t amr_prof_tick_getter;

void amr_prof_set_tick_getter(amr_prof_tick_getter_t getter)
{
    amr_prof_tick_getter = getter;
}

static amr_prof_tick_t amr_prof_now_cycles(void)
{
    amr_prof_tick_t tick_before;
    amr_prof_tick_t tick_after;
    amr_prof_tick_t timer_count;
    amr_prof_tick_t timer_period;

    if (amr_prof_tick_getter == 0) {
        return 0UL;
    }

    tick_before = amr_prof_tick_getter();
    timer_count = CpuTimer0Regs.TIM.all;
    tick_after = amr_prof_tick_getter();
    if (tick_after != tick_before) {
        tick_before = tick_after;
        timer_count = CpuTimer0Regs.TIM.all;
    }

    timer_period = CpuTimer0Regs.PRD.all;
    return tick_before * timer_period + (timer_period - timer_count);
}

void amr_prof_reset(void)
{
    Uint16 i;

    for (i = 0; i < AMR_PROF_STAGE_COUNT; i++) {
        amr_prof_cycles[i] = 0;
    }
}

amr_prof_tick_t amr_prof_start(void)
{
    return amr_prof_now_cycles();
}

void amr_prof_add(enum amr_prof_stage stage, amr_prof_tick_t start)
{
    amr_prof_tick_t end;

    if (stage >= AMR_PROF_STAGE_COUNT) {
        return;
    }

    end = amr_prof_now_cycles();
    amr_prof_cycles[stage] += (amr_prof_tick_t)(end - start);
}

static Uint32 amr_prof_us_per_frame(enum amr_prof_stage stage,
                                    amr_prof_tick_t frame_count)
{
    unsigned long long denom;

    if (frame_count == 0UL) {
        return 0;
    }

    denom = AMR_PROF_CPU_MHZ * (unsigned long long)frame_count;
    return (Uint32)(amr_prof_cycles[stage] / denom);
}

static void amr_prof_print_stage(char *name, enum amr_prof_stage stage,
                                 amr_prof_tick_t frame_count)
{
    UARTa_SendStringAndNumber(name,
                              (int32)amr_prof_us_per_frame(stage, frame_count),
                              "\r\n");
}

void amr_prof_report(amr_prof_tick_t frame_count)
{
    unsigned long long total_cycles = 0;
    Uint16 i;

    UARTa_SendStringAndNumber("AMR profile frames: ", (int32)frame_count, "\r\n");
    if (frame_count == 0UL) {
        return;
    }

    for (i = 0; i < AMR_PROF_STAGE_COUNT; i++) {
        total_cycles += amr_prof_cycles[i];
    }

    UARTa_SendStringAndNumber("prof total ms: ",
                              (int32)(total_cycles / (AMR_PROF_CPU_MHZ * 1000ULL)),
                              "\r\n");
    amr_prof_print_stage("prof preprocess us/frame: ", AMR_PROF_PREPROCESS,
                         frame_count);
    amr_prof_print_stage("prof lpc_lsp us/frame: ", AMR_PROF_LPC_LSP,
                         frame_count);
    amr_prof_print_stage("prof open_loop us/frame: ", AMR_PROF_OPEN_LOOP,
                         frame_count);
    amr_prof_print_stage("prof sf_pre us/frame: ", AMR_PROF_SF_PRE,
                         frame_count);
    amr_prof_print_stage("prof cl_ltp us/frame: ", AMR_PROF_CL_LTP,
                         frame_count);
    amr_prof_print_stage("prof cbsearch us/frame: ", AMR_PROF_CBSEARCH,
                         frame_count);
    amr_prof_print_stage("prof gain us/frame: ", AMR_PROF_GAIN,
                         frame_count);
    amr_prof_print_stage("prof sf_post us/frame: ", AMR_PROF_SF_POST,
                         frame_count);
    amr_prof_print_stage("prof tail us/frame: ", AMR_PROF_TAIL,
                         frame_count);
}

#endif
