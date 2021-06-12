/*
 * synth.c
 *
 *  Created on: May 30, 2021
 *      Author: mbruno
 */

#include <xcore/assert.h>
#include <dsp.h>
#include <rtos_printf.h>
#include <random.h>

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
        ch_state->envelope.stage_count = 4;
        ch_state->envelope.volume[0]     = Q24(1.0);
        ch_state->envelope.length[0]     =           (25 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[0] = Q24(1.0 / (25 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[1]     = Q24(0.75);
        ch_state->envelope.length[1]     =           (50 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[1] = Q24(1.0 / (50 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[2]     = Q24(0.0);
        ch_state->envelope.length[2]     =           (10000 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[2] = Q24(1.0 / (10000 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[3]     = Q24(0.0);
        ch_state->envelope.length[3]     =           (25 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[3] = Q24(1.0 / (25 * (SYNTH_SAMPLE_RATE/1000)));

    } else if (instrument == synth_instrument_sine) {
        ch_state->envelope.stage_count = 4;
        ch_state->envelope.volume[0]     = Q24(1.0);
        ch_state->envelope.length[0]     =           (25 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[0] = Q24(1.0 / (25 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[1]     = Q24(0.75);
        ch_state->envelope.length[1]     =           (50 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[1] = Q24(1.0 / (50 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[2]     = Q24(0.0);
        ch_state->envelope.length[2]     = 0;
        ch_state->envelope.length_inv[2] = 0;

        ch_state->envelope.volume[3]     = Q24(0.0);
        ch_state->envelope.length[3]     =           (25 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[3] = Q24(1.0 / (25 * (SYNTH_SAMPLE_RATE/1000)));

    } else if (instrument == synth_instrument_drum) {
        ch_state->envelope.stage_count = 3;
        ch_state->envelope.volume[0]     = Q24(1.0);
        ch_state->envelope.length[0]     =           (5 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[0] = Q24(1.0 / (5 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[1]     = Q24(0.0);
        ch_state->envelope.length[1]     =           (1000 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[1] = Q24(1.0 / (1000 * (SYNTH_SAMPLE_RATE/1000)));

        ch_state->envelope.volume[2]     = Q24(0.0);
        ch_state->envelope.length[2]     =           (100 * (SYNTH_SAMPLE_RATE/1000));
        ch_state->envelope.length_inv[2] = Q24(1.0 / (100 * (SYNTH_SAMPLE_RATE/1000)));
    }
}

void synth_channel_on(synth_state_t *synth_state, int channel, int32_t frequency, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (!ch_state->on) {
        /* These should probably already be set to 0 when the channel is turned off */
        ch_state->wave_table_index = 0;
        ch_state->envelope.cur_volume = 0;
    }
    if (!ch_state->on || ch_state->envelope.cur_stage >= ch_state->envelope.stage_count - 1) {
        ch_state->on = 1;
        ch_state->envelope.volume_scale_factor = velocity;

        const int stage = 0;
        ch_state->envelope.cur_stage = stage;
        ch_state->envelope.stage_time = 0;
        const int32_t target_volume = dsp_math_multiply(ch_state->envelope.volume_scale_factor, ch_state->envelope.volume[stage], 24);
        ch_state->envelope.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope.cur_volume, ch_state->envelope.length_inv[stage], 24);
    }

    /* Change the frequency even if the channel is already on */
    ch_state->frequency1 = frequency;
    ch_state->wave_table_step = dsp_math_multiply(frequency, Q16((float) SYNTH_WAVE_TABLE_SIZE / (float) SYNTH_SAMPLE_RATE), 16);
}

void synth_channel_off(synth_state_t *synth_state, int channel, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (ch_state->on && ch_state->envelope.cur_stage < ch_state->envelope.stage_count - 1) {
        const int stage = ch_state->envelope.stage_count - 1;
        ch_state->envelope.cur_stage = stage;
        ch_state->envelope.stage_time = 0;
        const int32_t target_volume = dsp_math_multiply(ch_state->envelope.volume_scale_factor, ch_state->envelope.volume[stage], 24);
        ch_state->envelope.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope.cur_volume, ch_state->envelope.length_inv[stage], 24);
    }
}

int8_t sample_get_next(synth_state_t *synth_state, int channel)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    int32_t i;
    int8_t sample;

    if (!ch_state->on) {
        return 0;
    }

    int stage = ch_state->envelope.cur_stage;
    ch_state->envelope.stage_time++;

    ch_state->envelope.cur_volume += ch_state->envelope.cur_rate;
    if (ch_state->envelope.cur_volume >= Q24(1.0)) {
        ch_state->envelope.cur_volume = Q24(1.0);
    }
    if (ch_state->envelope.cur_volume <= Q24(0.0)) {
        ch_state->envelope.cur_volume = Q24(0.0);
    }
    if (ch_state->envelope.stage_time >= ch_state->envelope.length[stage] && ch_state->envelope.length[stage] > 0) {
        stage++;
        if (stage < ch_state->envelope.stage_count) {
            ch_state->envelope.stage_time = 0;
            const int32_t target_volume = dsp_math_multiply(ch_state->envelope.volume_scale_factor, ch_state->envelope.volume[stage], 24);
            ch_state->envelope.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope.cur_volume, ch_state->envelope.length_inv[stage], 24);
        } else {
            stage = 0;
            ch_state->on = 0;
        }
        ch_state->envelope.cur_stage = stage;
    }


    i = ch_state->wave_table_index + Q16(0.5);
    i >>= 16;

    /* TODO: Interpolate */
    sample = synth_wave_table[ch_state->instrument][i];

    if (ch_state->instrument == synth_instrument_drum) {
        static random_generator_t seed;
        int32_t r;
        r = random_get_random_number(&seed);

        r >>= 24;
        sample += r/8;
    }

    ch_state->wave_table_index += ch_state->wave_table_step;
    while (ch_state->wave_table_index >= Q16(SYNTH_WAVE_TABLE_SIZE)) {
        ch_state->wave_table_index -= Q16(SYNTH_WAVE_TABLE_SIZE);
    }

    sample = dsp_math_multiply(sample, ch_state->envelope.cur_volume, 0+24-0);

    /*
     * Scale each channel to -6 dBFS. This should result in mixing that rarely clips.
     */
//    sample = dsp_math_multiply(sample, Q24(0.5), 0+24-0);

    return sample;
}
