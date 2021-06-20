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

typedef struct {
    int stage_count;
    int32_t volume[8];
    int32_t length_inv[8]; /* 1/length of stage, where length is sample count */
    int32_t length[8]; /* length of stage, where length is sample count */
} synth_channel_envelope_t;

typedef struct {
    int on;
    int32_t channel_volume;
    synth_channel_envelope_t envelope;
    struct {
        int cur_stage;
        int32_t volume_scale_factor; /* MIDI ON velocity */
        int32_t cur_rate;
        int32_t cur_volume;
        int32_t stage_time;
    } envelope_state;

    int32_t frequency;
    int32_t wave_table_step_final;
    int32_t wave_table_step_sweep_increment;

    int sweeping;
    int noise_enabled;
    int32_t sweep_range;
    int32_t sweep_length;
    int sweep_direction;

    const int8_t *wave_table;
    int32_t wave_table_step;
    size_t wave_table_index;
    
} synth_channel_state_t;

typedef struct {
    synth_channel_state_t channel_state[SYNTH_CHANNELS];
} synth_state_t;


void synth_envelope_create(synth_channel_envelope_t *envelope, int stage_count, int32_t volume[], int32_t length[]);
void synth_channel_volume_set(synth_state_t *synth_state, int channel, int32_t volume);
void synth_channel_envelope_set(synth_state_t *synth_state, int channel, synth_channel_envelope_t *envelope);
void synth_channel_sweep_set(synth_state_t *synth_state, int channel, int32_t sweep_range, int sweep_direction, int32_t sweep_length);
void synth_channel_sweep_off(synth_state_t *synth_state, int channel);
void synth_channel_noise_on(synth_state_t *synth_state, int channel);
void synth_channel_noise_off(synth_state_t *synth_state, int channel);
void synth_channel_wave_table_set(synth_state_t *synth_state, int channel, const int8_t *wave_table);

void synth_channel_on(synth_state_t *synth_state, int channel, int32_t frequency, int32_t velocity);
void synth_channel_frequency_change(synth_state_t *synth_state, int channel, int32_t frequency, int32_t transition_length);
void synth_channel_off(synth_state_t *synth_state, int channel, int32_t velocity);

int8_t sample_get_next(synth_state_t *synth_state, int channel);

#endif /* SRC_SYNTH_H_ */
