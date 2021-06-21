/*
 * synth.c
 *
 *  Created on: May 30, 2021
 *      Author: mbruno
 */

#include <string.h>
#include <xcore/assert.h>
#include <dsp.h>
#include <rtos_printf.h>
#include <random.h>

#include "synth.h"
#include "synth_wave_table.h"

#define DR 48.0

static synth_channel_state_t *channel_state_get(synth_state_t *synth_state, int channel)
{
    xassert(channel >= 0 && channel < SYNTH_CHANNELS);

    return &synth_state->channel_state[channel];
}

void synth_envelope_create(synth_channel_envelope_t *envelope, int stage_count, int32_t volume[], int32_t length[])
{
    envelope->stage_count = stage_count;

    for (int i = 0; i < stage_count; i++) {
        envelope->volume[i] = volume[i];
        if (length[i] == INT32_MAX) {
            envelope->length[i] = INT32_MAX;
            envelope->length_inv[i] = 0;
        } else if (length[i] == 0) {
            envelope->length[i] = 0;
            envelope->length_inv[i] = Q24(1);
        } else {
            envelope->length[i] = length[i] * (SYNTH_SAMPLE_RATE/1000);
            envelope->length_inv[i] = dsp_math_divide(Q24(1), envelope->length[i], 0);
        }
    }
}

void synth_channel_volume_set(synth_state_t *synth_state, int channel, int32_t volume)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->channel_volume = volume;
}

void synth_channel_envelope_set(synth_state_t *synth_state, int channel, synth_channel_envelope_t *envelope)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    memcpy(&ch_state->envelope, envelope, sizeof(synth_channel_envelope_t));
}

void synth_channel_sweep_set(synth_state_t *synth_state, int channel, int32_t sweep_range, int sweep_direction, int32_t sweep_length)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->sweep_range = sweep_range;
    ch_state->sweep_length = sweep_length;
    ch_state->sweep_direction = sweep_direction;
}

void synth_channel_sweep_off(synth_state_t *synth_state, int channel)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->sweep_direction = 0;
}

void synth_channel_noise_on(synth_state_t *synth_state, int channel)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->noise_enabled = 1;
}

void synth_channel_noise_off(synth_state_t *synth_state, int channel)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->noise_enabled = 0;
}

void synth_channel_wave_table_set(synth_state_t *synth_state, int channel, const int8_t *wave_table)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    ch_state->wave_table = wave_table;
}

void synth_channel_frequency_change(synth_state_t *synth_state, int channel, int32_t frequency, int32_t transition_length)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (ch_state->on) {
        int32_t wave_table_step = dsp_math_multiply(frequency, Q16((float) SYNTH_WAVE_TABLE_SIZE / (float) SYNTH_SAMPLE_RATE), 16);

        if (transition_length > 0) {
            ch_state->wave_table_step_final = wave_table_step;

            ch_state->wave_table_step_sweep_increment =
                    dsp_math_divide(wave_table_step - ch_state->wave_table_step, transition_length * (SYNTH_SAMPLE_RATE / 1000), 0);

            if (wave_table_step != ch_state->wave_table_step) {
                ch_state->sweeping = 1;
            } else {
                ch_state->sweeping = 0;
            }
        } else {
            ch_state->wave_table_step = wave_table_step;
            ch_state->sweeping = 0;
        }
    }
}

void synth_channel_on(synth_state_t *synth_state, int channel, int32_t frequency, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (!ch_state->on || ch_state->envelope_state.cur_stage >= ch_state->envelope.stage_count - 1) {

        if (!ch_state->on) {
            /* These should probably already be set to 0 when the channel is turned off */
            ch_state->wave_table_index = 0;
            ch_state->envelope_state.cur_volume = Q24(-DR);
        }

        ch_state->on = 1;

        ch_state->envelope_state.volume_scale_factor = velocity + ch_state->envelope.volume[0];

        const int stage = 0;
        ch_state->envelope_state.cur_stage = stage;
        ch_state->envelope_state.stage_time = 0;
        const int32_t target_volume = ch_state->envelope_state.volume_scale_factor;
        ch_state->envelope_state.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope_state.cur_volume, ch_state->envelope.length_inv[stage], 24);

        int32_t initial_frequency;
        int32_t end_frequency;

        if (ch_state->sweep_direction != 0) {
            if (ch_state->sweep_direction > 0) {
                initial_frequency = frequency;
                end_frequency = dsp_math_multiply(frequency, ch_state->sweep_range, 16);
            } else {
                initial_frequency = dsp_math_multiply(frequency, ch_state->sweep_range, 16);
                end_frequency = frequency;
            }
        } else {
            initial_frequency = frequency;
        }
        ch_state->frequency = frequency;
        ch_state->wave_table_step = dsp_math_multiply(initial_frequency, Q16((float) SYNTH_WAVE_TABLE_SIZE / (float) SYNTH_SAMPLE_RATE), 16);

        if (ch_state->sweep_direction != 0) {
            synth_channel_frequency_change(synth_state, channel, end_frequency, ch_state->sweep_length);
        } else {
            ch_state->sweeping = 0;
        }
    }
}

/*
 * TODO: velocity currently unused. do something with it? remove it?
 */
void synth_channel_off(synth_state_t *synth_state, int channel, int32_t velocity)
{
    synth_channel_state_t *ch_state = channel_state_get(synth_state, channel);

    if (ch_state->on && ch_state->envelope_state.cur_stage < ch_state->envelope.stage_count - 1) {
        const int stage = ch_state->envelope.stage_count - 1;
        ch_state->envelope_state.cur_stage = stage;
        ch_state->envelope_state.stage_time = 0;
        const int32_t target_volume = ch_state->envelope_state.volume_scale_factor + ch_state->envelope.volume[stage];
        ch_state->envelope_state.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope_state.cur_volume, ch_state->envelope.length_inv[stage], 24);
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

    int stage = ch_state->envelope_state.cur_stage;
    ch_state->envelope_state.stage_time++;

    ch_state->envelope_state.cur_volume += ch_state->envelope_state.cur_rate;
    if (ch_state->envelope_state.cur_volume >= Q24(0.0)) {
        ch_state->envelope_state.cur_volume = Q24(0.0);
    }
    if (ch_state->envelope_state.cur_volume <= Q24(-DR)) {
        ch_state->envelope_state.cur_volume = Q24(-DR);
    }
    if (ch_state->envelope_state.stage_time >= ch_state->envelope.length[stage]) {
        stage++;
        if (stage < ch_state->envelope.stage_count) {
            ch_state->envelope_state.stage_time = 0;
            const int32_t target_volume = ch_state->envelope_state.volume_scale_factor + ch_state->envelope.volume[stage];
            ch_state->envelope_state.cur_rate = dsp_math_multiply(target_volume - ch_state->envelope_state.cur_volume, ch_state->envelope.length_inv[stage], 24);
        } else {
            stage = 0;
            ch_state->on = 0;
        }
        ch_state->envelope_state.cur_stage = stage;
    }


    i = ch_state->wave_table_index + Q16(0.5);
    i >>= 16;
    if (i >= SYNTH_WAVE_TABLE_SIZE) {
        i -= SYNTH_WAVE_TABLE_SIZE;
    }
    xassert(i >= 0 && i < SYNTH_WAVE_TABLE_SIZE);

    /* TODO: Interpolate */
    sample = ch_state->wave_table[i];

    if (ch_state->noise_enabled) {
        static random_generator_t seed;
        int32_t r = random_get_random_number(&seed);
        int8_t orig_samp = sample;

        r >>= 24;
        sample += r/8;

        if (orig_samp > 0 && r > 0 && sample < 0) {
            sample = INT8_MAX;
        } else if (orig_samp < 0 && r < 0 && sample > 0) {
            sample = INT8_MIN;
        }
    }

    ch_state->wave_table_index += ch_state->wave_table_step;
    while (ch_state->wave_table_index >= Q16(SYNTH_WAVE_TABLE_SIZE)) {
        ch_state->wave_table_index -= Q16(SYNTH_WAVE_TABLE_SIZE);
    }

    if (ch_state->sweeping) {
        ch_state->wave_table_step += ch_state->wave_table_step_sweep_increment;
        if ((ch_state->wave_table_step_sweep_increment > 0 && ch_state->wave_table_step >= ch_state->wave_table_step_final) ||
                (ch_state->wave_table_step_sweep_increment < 0 && ch_state->wave_table_step <= ch_state->wave_table_step_final)) {
            ch_state->sweeping = 0;
        }
    }


    const double ln10 = 2.30258509299;
    int32_t m1 = ch_state->channel_volume;
    int32_t m2 = ch_state->envelope_state.cur_volume;

    int32_t sample_multiplier = dsp_math_exp(dsp_math_multiply(m1 + m2, Q24(ln10 / 20.0), 24));

    sample = dsp_math_multiply(sample, sample_multiplier, 0+24-0);

    return sample;
}
