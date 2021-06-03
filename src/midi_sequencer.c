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

static int32_t note_frequency_lookup[128];

static synth_state_t synth_state;

static int channel_program[16];
static synth_instrument_t channel_instrument[16];

typedef struct {
    int midi_channel;
    int note;
    int on;
} channel_info_t;

channel_info_t channel_info[SYNTH_CHANNELS];

void midi_sequencer_reset(void)
{
    for (int i = 0; i < 128; i++) {
        note_frequency_lookup[i] = Q16(440.0 * pow(2, (i - 69.0) / 12.0));
    }

    memset(&synth_state, 0, sizeof(synth_state));
    memset(channel_info, 0, sizeof(channel_info));
}

void midi_sequencer_note_on(int channel, int note_number, int velocity)
{
    int32_t frequency;
    int32_t synth_velocity;
    int synth_channel;

    xassert(channel >= 0 && channel < 16);
    xassert(note_number >= 0 && note_number < 128);

    synth_velocity = dsp_math_multiply(velocity, Q24(1.0/127.0), 0+24-24);

    if (channel_instrument[channel] != synth_instrument_drum) {
        frequency = note_frequency_lookup[note_number];
    } else {
        frequency = Q16(70.0);
    }

    /*
     * Determine the channel to play the note on by finding the first
     * channel that is off. If no channels are off, then find the first
     * channel that is released. Otherwise, print a message.
     */

    for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
        synth_channel_state_t *ch_state;
        ch_state = &synth_state.channel_state[synth_channel];
        if (ch_state->mode == SYNTH_CHANNEL_MODE_OFF) {
            break;
        }
    }

    if (synth_channel == SYNTH_CHANNELS) {
        for (synth_channel = 0; synth_channel < SYNTH_CHANNELS; synth_channel++) {
            synth_channel_state_t *ch_state;
            ch_state = &synth_state.channel_state[synth_channel];
            if (ch_state->mode == SYNTH_CHANNEL_MODE_RELEASED) {
                break;
            }
        }
    }

    if (synth_channel < SYNTH_CHANNELS) {
        xassert(!channel_info[synth_channel].on);
        channel_info[synth_channel].note = note_number;
        channel_info[synth_channel].midi_channel = channel;
        channel_info[synth_channel].on = 1;

        synth_channel_instrument_set(&synth_state, synth_channel, channel_instrument[channel]);
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

synth_instrument_t program_to_instrument(int program_number)
{
    switch (program_number + 1) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        return synth_instrument_piano;
    default:
        return synth_instrument_piano;
    }
}

void midi_sequencer_program_change(int channel, int program_number)
{
    xassert(program_number >= 0 && program_number < 128);
    xassert(channel >= 0 && channel < 16);

    channel_program[channel] = program_number;

    if (channel != 9) {
        channel_instrument[channel] = program_to_instrument(program_number);
    } else {
        /* Drums */
        channel_instrument[channel] = synth_instrument_drum;
    }
}

synth_state_t * midi_sequencer_synth_state(void)
{
    return &synth_state;
}
