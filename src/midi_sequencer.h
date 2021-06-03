/*
 * midi_sequencer.h
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#ifndef MIDI_SEQUENCER_H_
#define MIDI_SEQUENCER_H_

#include "synth/synth.h"

void midi_sequencer_reset(void);
void midi_sequencer_note_on(int channel, int note_number, int velocity);
void midi_sequencer_note_off(int channel, int note_number, int velocity);
void midi_sequencer_program_change(int channel, int program_number);
synth_state_t * midi_sequencer_synth_state(void);

#endif /* MIDI_SEQUENCER_H_ */
