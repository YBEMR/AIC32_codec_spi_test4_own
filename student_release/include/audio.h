#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <stdint.h>

#include "DSP2833x_Device.h"   
#include "DSP2833x_Examples.h" 

void AIC23Init(void);
void I2CA_Init(void);

typedef Uint32 (*audio_tick_getter_t)(void);
void audio_set_tick_getter(audio_tick_getter_t getter);
void create_wav_header(uint8_t *header, uint32_t data_size);

#endif  //_AUDIO_H_
