/*
 * midi_sequencer.c
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#include <string.h>
#include <rtos_printf.h>
#include <xcore/assert.h>
#include <math.h>
#include <dsp.h>
#include "synth/synth.h"
#include "synth/synth_wave_table.h"

#define PITCH_BEND_LOOKUP 0

static int32_t note_frequency_lookup[128];

/*
 * this table is large. probably ok to calculate on demand
 */
#if PITCH_BEND_LOOKUP
static int32_t pitch_bend_lookup[16384];
#endif

static int32_t pitch_bend_multiplier(int pitch_bend)
{
    /*
     * The pitch can be bent by multiplying the base note frequency with:
     *
     * 2^((pitchbend - 8192) / (4096 * 12))
     *
     * or, with only exp():
     *
     * e^((pitchbend - 8192) * (ln(2) / (4096 * 12)))
     *
     * https://dsp.stackexchange.com/questions/1645/converting-a-pitch-bend-midi-value-to-a-normal-pitch-value
     */

#if PITCH_BEND_LOOKUP
    return pitch_bend_lookup[pitch_bend];
#else
    const double ln2 = 0.69314718056;
    q8_24 exponent = dsp_math_multiply(Q16(pitch_bend - 8192), Q31(ln2 / (4096.0 * 12.0)), 16+31-24);
    return (dsp_math_exp(exponent) + Q8(0.5)) >> 8;
#endif
}

static synth_state_t synth_state;

static int channel_program[16];
static int channel_pitch_bend[16];

typedef struct {
    int midi_channel;
    int note;
    int on;
} channel_info_t;

channel_info_t channel_info[SYNTH_CHANNELS];

static synth_channel_envelope_t default_envelope;
static synth_channel_envelope_t drum_envelope;

void midi_sequencer_reset(void)
{
    const double ln2 = 0.69314718056;

    for (int i = 0; i < 128; i++) {

        /*
         * Note frequencies are calculated with the formula:
         *
         * freq = 440Hz * 2^((note - 69) / 12)
         *
         * lib_dsp only provides exp(), so:
         *
         * freq = 440Hz * e^(ln(2) * (note - 69) / 12)
         * freq = 440Hz * e^((note - 69) * (ln(2) / 12))
         */

        //note_frequency_lookup[i] = Q16(440.0 * pow(2, (i - 69.0) / 12.0));
        //printf("%d: %f\n", i, F16(note_frequency_lookup[i]));

        q8_24 exponent = dsp_math_multiply(Q24(i - 69), Q24(ln2 / 12.0), 24);
        q8_24 multiplier = dsp_math_exp(exponent);
        note_frequency_lookup[i] = dsp_math_multiply(Q16(440.0), multiplier, 16+24-16);
        //printf("%d: %f\n\n", i, F16(note_frequency_lookup[i]));
    }

#if PITCH_BEND_LOOKUP
    for (int i = 0; i < 16384; i++) {

        /*
         * The pitch can be bent by multiplying the base note frequency with:
         *
         * 2^((pitchbend - 8192) / (4096 * 12))
         *
         * or, with only exp():
         *
         * e^((pitchbend - 8192) * (ln(2) / (4096 * 12)))
         */

        //pitch_bend_lookup[i] = Q16(pow(2, (i - 8192.0) / (4096.0 * 12.0)));
        //printf("%d: %f\n", i, F16(pitch_bend_lookup[i]));

        q8_24 exponent = dsp_math_multiply(Q16(i - 8192), Q31(ln2 / (4096.0 * 12.0)), 16+31-24);
        pitch_bend_lookup[i] = (dsp_math_exp(exponent) + Q8(0.5)) >> 8;
        //printf("%d: %f\n\n", i, F16(pitch_bend_lookup[i]));
    }
#endif

    int32_t volume[4];
    int32_t length[4];

    volume[0] = Q24(0.25); length[0] = 25;
    volume[1] = Q24(0.75); length[1] = 50;
    volume[2] = Q24(0.00); length[2] = 10000;
    volume[3] = Q24(0.00); length[3] = 25;
    synth_envelope_create(&default_envelope, 4, volume, length);

    volume[0] = Q24(0.25); length[0] = 0;
    volume[1] = Q24(0.00); length[1] = 250;
    volume[2] = Q24(0.00); length[2] = 50;
    synth_envelope_create(&drum_envelope, 3, volume, length);

    memset(&synth_state, 0, sizeof(synth_state));
    memset(channel_info, 0, sizeof(channel_info));
    memset(channel_program, 0, sizeof(channel_program));

    for (int i = 0; i < 16; i++) {
        channel_pitch_bend[i] = 8192;
    }
}

static void synth_channel_setup(int synth_channel, int midi_channel)
{
    int program = channel_program[midi_channel];

    const int8_t *wave_table = synth_wave_table[synth_waveform_pulse_12p5];
    synth_channel_envelope_t *envelope = &default_envelope;
    int noise_enabled = 0;
    int sweep_direction = 0;
    int32_t sweep_range;
    int32_t sweep_length;
    int drum = 0;

    if (midi_channel == 9) {
        drum = 1;
    }

    switch (program + 1) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        wave_table = synth_wave_table[synth_waveform_piano];
        break;
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        wave_table = synth_wave_table[synth_waveform_triangle];
        break;
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
        wave_table = synth_wave_table[synth_waveform_triangle];
        break;
    case 81:
        wave_table = synth_wave_table[synth_waveform_pulse_50];
        break;
    case 82:
        wave_table = synth_wave_table[synth_waveform_sawtooth];
        break;
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 120:
    case 128:
        drum = 1;
        break;
    }

    if (drum) {
        wave_table = synth_wave_table[synth_waveform_triangle];
        envelope = &drum_envelope;
        noise_enabled = 1;
        sweep_direction = 1;
        sweep_range = Q16(0.1);
        sweep_length = 35;
    }

    synth_channel_wave_table_set(&synth_state, synth_channel, wave_table);
    synth_channel_envelope_set(&synth_state, synth_channel, envelope);
    if (noise_enabled) {
        synth_channel_noise_on(&synth_state, synth_channel);
    } else {
        synth_channel_noise_off(&synth_state, synth_channel);
    }
    if (sweep_direction) {
        synth_channel_sweep_set(&synth_state, synth_channel, sweep_range, sweep_direction, sweep_length);
    } else {
        synth_channel_sweep_off(&synth_state, synth_channel);
    }
}

void midi_sequencer_note_on(int channel, int note_number, int velocity)
{
    int32_t frequency;
    int32_t synth_velocity;
    int synth_channel;

    xassert(channel >= 0 && channel < 16);
    xassert(note_number >= 0 && note_number < 128);

    synth_velocity = dsp_math_multiply(velocity, Q24(1.0/127.0), 0+24-24);

    if (channel != 9) {
        frequency = note_frequency_lookup[note_number];
        if (channel_pitch_bend[channel] != 8192) {
            xassert(channel_pitch_bend[channel] >= 0 && channel_pitch_bend[channel] < 16384);
            frequency = dsp_math_multiply(frequency, pitch_bend_multiplier(channel_pitch_bend[channel]), 16);
        }
    } else {
        /*
         * On channel 9, the note number is the type of percussion instrument,
         * so does not make sense to lookup note frequency.
         * For the percussion instrument programs 113-128, however,
         * looking up the note does make sense.
         */
        frequency = Q16(523.25);
    }

    /*
     * Determine the channel to play the note on by finding the first
     * channel that is off. If no channels are off, then find the first
     * channel that is released. Otherwise, print a message.
     */

    for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
        synth_channel_state_t *ch_state;
        ch_state = &synth_state.channel_state[synth_channel];
        if (!ch_state->on) {
            break;
        }
    }

    if (synth_channel == SYNTH_CHANNELS) {
        for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
            synth_channel_state_t *ch_state;
            ch_state = &synth_state.channel_state[synth_channel];
            if (ch_state->envelope_state.cur_stage >= ch_state->envelope.stage_count - 1) {
                break;
            }
        }
    }

    if (synth_channel < SYNTH_CHANNELS) {
        channel_info[synth_channel].note = note_number;
        channel_info[synth_channel].midi_channel = channel;
        channel_info[synth_channel].on = 1;

        synth_channel_setup(synth_channel, channel);
        synth_channel_on(&synth_state, synth_channel, frequency, synth_velocity);
    } else {
        rtos_printf("Cannot play more than %d notes at a time\n", SYNTH_CHANNELS);
    }
}

void midi_sequencer_note_off(int channel, int note_number, int velocity)
{
    int synth_channel;

    xassert(channel >= 0 && channel < 16);

    /*
     * Search through the channel_info array to find the channel
     * that is playing the note that must be turned off.
     */
    for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
        if (channel_info[synth_channel].on && channel_info[synth_channel].note == note_number && channel_info[synth_channel].midi_channel == channel) {
            int32_t synth_velocity = dsp_math_multiply(velocity, Q24(1.0/127.0), 0+24-24);
            synth_channel_off(&synth_state, synth_channel, synth_velocity);
            channel_info[synth_channel].on = 0;
            return;
        }
    }
}

void midi_sequencer_pitch_change(int channel, int pitch)
{
    int synth_channel;

    xassert(pitch >= 0 && pitch < 16384);
    xassert(channel >= 0 && channel < 16);

    channel_pitch_bend[channel] = pitch;

    /*
     * Apply the bend to all active synth channels that are
     * associated with this MIDI channel.
     */
    for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
        if (channel_info[synth_channel].midi_channel == channel) {
            synth_channel_state_t *ch_state;
            ch_state = &synth_state.channel_state[synth_channel];
            if (ch_state->on) {
                int32_t frequency = note_frequency_lookup[channel_info[synth_channel].note];
                frequency = dsp_math_multiply(frequency, pitch_bend_multiplier(pitch), 16);
                synth_channel_frequency_change(&synth_state, synth_channel, frequency, 1);
            }
        }
    }
}

void midi_sequencer_program_change(int channel, int program_number)
{
    xassert(program_number >= 0 && program_number < 128);
    xassert(channel >= 0 && channel < 16);

    channel_program[channel] = program_number;
}

synth_state_t * midi_sequencer_synth_state(void)
{
    return &synth_state;
}
