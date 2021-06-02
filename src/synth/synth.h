/*
 * synth.h
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#ifndef SRC_SYNTH_H_
#define SRC_SYNTH_H_

#include <stddef.h>
#include <stdint.h>

#define SYNTH_SAMPLE_RATE 16000
#define SYNTH_CHANNELS 16
#define SYNTH_WAVE_TABLE_SIZE 128

#define SYNTH_CHANNEL_MODE_ATTACK_1_LENGTH (25 * (SYNTH_SAMPLE_RATE/1000))
#define SYNTH_CHANNEL_MODE_ATTACK_2_LENGTH (50 * (SYNTH_SAMPLE_RATE/1000))
#define SYNTH_CHANNEL_MODE_RELEASED_LENGTH (10 * (SYNTH_SAMPLE_RATE/1000))

typedef enum {
    synth_instrument_sine,
    synth_instrument_piano,

} synth_instrument_t;

typedef struct {
    synth_instrument_t instrument;
    enum {SYNTH_CHANNEL_MODE_OFF, SYNTH_CHANNEL_MODE_ATTACK_1, SYNTH_CHANNEL_MODE_ATTACK_2, SYNTH_CHANNEL_MODE_SUSTAIN, SYNTH_CHANNEL_MODE_RELEASED} mode;
    int32_t frequency;
    int32_t wave_table_step;
    size_t wave_table_index;
    int32_t mode_time;
    int32_t velocity;
    int32_t attack_decay_rate;
    int32_t sustain_decay_rate;
    int32_t volume;
} synth_channel_state_t;

typedef struct {
    synth_channel_state_t channel_state[SYNTH_CHANNELS];
} synth_state_t;


void synth_channel_instrument_set(synth_state_t *synth_state, int channel, synth_instrument_t instrument);
void synth_channel_on(synth_state_t *synth_state, int channel, int32_t frequency, int32_t velocity);
void synth_channel_off(synth_state_t *synth_state, int channel, int32_t velocity);
int8_t sample_get_next(synth_state_t *synth_state, int channel);

#endif /* SRC_SYNTH_H_ */
