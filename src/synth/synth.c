/*
 * synth.c
 *
 *  Created on: May 30, 2021
 *      Author: mbruno
 */

#include <xcore/assert.h>
#include <dsp.h>
#include <rtos_printf.h>

#include "synth.h"
#include "synth_wave_table.h"


static synth_channel_state_t * channel_state_get(synth_state_t *synth_state, int channel)
{
    xassert(channel >= 0 && channel < SYNTH_CHANNELS);

    return &synth_state->channel_state[channel];
}

void synth_channel_instrument_set(synth_state_t *synth_state, int channel, synth_instrument_t instrument)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->instrument = instrument;

    if (instrument == synth_instrument_piano) {
#define PIANO_ON_LENGTH (10000 * (SYNTH_SAMPLE_RATE/1000))
#define PIANO_ON_LENGTH (10000 * (SYNTH_SAMPLE_RATE/1000))
        ch_state->attack_decay_rate = Q24(1.0 / (2*SYNTH_CHANNEL_MODE_ATTACK_2_LENGTH));
        ch_state->sustain_decay_rate = Q24(1.0 / PIANO_ON_LENGTH);
    }
    if (instrument == synth_instrument_sine) {
        ch_state->attack_decay_rate = 0;
        ch_state->sustain_decay_rate = 0;
    }
}

void synth_channel_on(synth_state_t *synth_state, int channel, int32_t frequency, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (ch_state->mode == SYNTH_CHANNEL_MODE_OFF || ch_state->mode == SYNTH_CHANNEL_MODE_RELEASED) {
        ch_state->wave_table_index = 0;
        ch_state->mode = SYNTH_CHANNEL_MODE_ATTACK_1;
        ch_state->mode_time = 0;
        ch_state->velocity = dsp_math_multiply(velocity, Q24(1.0 / SYNTH_CHANNEL_MODE_ATTACK_1_LENGTH), 24);
    }

    ch_state->frequency = frequency;
    ch_state->wave_table_step = dsp_math_multiply(frequency, Q16((float) SYNTH_WAVE_TABLE_SIZE / (float) SYNTH_SAMPLE_RATE), 16);
}

void synth_channel_off(synth_state_t *synth_state, int channel, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (ch_state->mode != SYNTH_CHANNEL_MODE_OFF && ch_state->mode != SYNTH_CHANNEL_MODE_RELEASED) {
        ch_state->mode = SYNTH_CHANNEL_MODE_RELEASED;
        ch_state->mode_time = 0;
        ch_state->velocity = dsp_math_multiply(velocity, Q24(1.0 / SYNTH_CHANNEL_MODE_RELEASED_LENGTH), 24);
    }
}

int8_t sample_get_next(synth_state_t *synth_state, int channel)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    int32_t i;
    int8_t sample;

    switch (ch_state->mode) {
    case SYNTH_CHANNEL_MODE_OFF:
        return 0;
    case SYNTH_CHANNEL_MODE_ATTACK_1:
        ch_state->mode_time++;
        ch_state->volume += ch_state->velocity;

        if (ch_state->volume >= Q24(1.0)) {
            ch_state->volume = Q24(1.0);
        }

        if (ch_state->mode_time >= SYNTH_CHANNEL_MODE_ATTACK_1_LENGTH || ch_state->volume >= Q24(1.0)) {
            ch_state->mode = SYNTH_CHANNEL_MODE_ATTACK_2;
            ch_state->mode_time = 0;
        }

        break;

    case SYNTH_CHANNEL_MODE_ATTACK_2:
        ch_state->mode_time++;
        ch_state->volume -= ch_state->attack_decay_rate;

        if (ch_state->volume <= Q24(0.0)) {
            ch_state->volume = Q24(0.0);
        }

        if (ch_state->mode_time >= SYNTH_CHANNEL_MODE_ATTACK_2_LENGTH || ch_state->volume <= Q24(0.0)) {
            ch_state->mode = SYNTH_CHANNEL_MODE_SUSTAIN;
            ch_state->mode_time = 0;
        }

        break;

    case SYNTH_CHANNEL_MODE_SUSTAIN:
        ch_state->volume -= ch_state->sustain_decay_rate;
        if (ch_state->volume <= Q24(0.0)) {
            ch_state->volume = Q24(0.0);
        }
        break;

    case SYNTH_CHANNEL_MODE_RELEASED:

        ch_state->volume -= ch_state->velocity;
        if (ch_state->volume <= Q24(0.0)) {
            ch_state->volume = Q24(0.0);
            ch_state->mode = SYNTH_CHANNEL_MODE_OFF;
        }
        break;
    default:
        return 0;
    }

    i = ch_state->wave_table_index + Q16(0.5);
    i >>= 16;

    /* TODO: Interpolate */
    sample = synth_wave_table[ch_state->instrument][i];

    ch_state->wave_table_index += ch_state->wave_table_step;
    while (ch_state->wave_table_index >= Q16(SYNTH_WAVE_TABLE_SIZE)) {
        ch_state->wave_table_index -= Q16(SYNTH_WAVE_TABLE_SIZE);
    }

    sample = dsp_math_multiply(sample, ch_state->volume, 0+24-0);

    /*
     * Scale each channel to -6 dBFS. This should result in mixing that rarely clips.
     */
    sample = dsp_math_multiply(sample, Q24(0.5), 0+24-0);

    return sample;
}
