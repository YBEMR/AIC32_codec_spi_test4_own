#ifndef AMR_PROFILE_H
#define AMR_PROFILE_H

#ifndef AMR_PROFILE
#define AMR_PROFILE 0
#endif

typedef unsigned long amr_prof_tick_t;
typedef amr_prof_tick_t (*amr_prof_tick_getter_t)(void);

enum amr_prof_stage {
    AMR_PROF_PREPROCESS = 0,
    AMR_PROF_LPC_LSP,
    AMR_PROF_OPEN_LOOP,
    AMR_PROF_SF_PRE,
    AMR_PROF_CL_LTP,
    AMR_PROF_CBSEARCH,
    AMR_PROF_GAIN,
    AMR_PROF_SF_POST,
    AMR_PROF_TAIL,
    AMR_PROF_STAGE_COUNT
};

#if AMR_PROFILE
void amr_prof_reset(void);
void amr_prof_set_tick_getter(amr_prof_tick_getter_t getter);
amr_prof_tick_t amr_prof_start(void);
void amr_prof_add(enum amr_prof_stage stage, amr_prof_tick_t start);
void amr_prof_report(amr_prof_tick_t frame_count);
#else
#define amr_prof_reset() ((void)0)
#define amr_prof_set_tick_getter(getter) ((void)(getter))
#define amr_prof_start() (0UL)
#define amr_prof_add(stage, start) ((void)(stage), (void)(start))
#define amr_prof_report(frame_count) ((void)(frame_count))
#endif

#endif
