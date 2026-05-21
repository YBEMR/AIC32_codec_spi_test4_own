#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "DSP2833x_Device.h"   
#include "DSP2833x_Examples.h" 

void AIC23Init(void);
void I2CA_Init(void);

typedef Uint32 (*audio_tick_getter_t)(void);
void audio_set_tick_getter(audio_tick_getter_t getter);

#endif  //_AUDIO_H_
