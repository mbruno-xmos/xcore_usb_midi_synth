/*
 * synth_wave_table.h
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#ifndef SYNTH_WAVE_TABLE_H_
#define SYNTH_WAVE_TABLE_H_

typedef enum {
    synth_waveform_sine,
    synth_waveform_pulse_12p5,
    synth_waveform_pulse_25,
    synth_waveform_pulse_50,
    synth_waveform_pulse_75,
    synth_waveform_triangle,
    synth_waveform_sawtooth,
    synth_waveform_piano,

} synth_waveform_t;

extern const int8_t synth_wave_table[][SYNTH_WAVE_TABLE_SIZE];

#endif /* SYNTH_WAVE_TABLE_H_ */
