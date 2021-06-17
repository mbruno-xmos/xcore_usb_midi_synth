/*
 * synth_wave_table.c
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#include "synth.h"
#include "synth_wave_table.h"


const int8_t synth_wave_table[][SYNTH_WAVE_TABLE_SIZE] = {
        [synth_waveform_sine] = {
                0,6,12,19,25,31,37,43,49,54,60,65,71,76,81,85,90,94,98,102,106,109,112,115,117,120,122,123,125,126,126,127,127,127,126,126,125,123,122,120,117,115,112,109,106,102,98,94,90,85,81,76,71,65,60,54,49,43,37,31,25,19,12,6,0,-6,-12,-19,-25,-31,-37,-43,-49,-54,-60,-65,-71,-76,-81,-85,-90,-94,-98,-102,-106,-109,-112,-115,-117,-120,-122,-123,-125,-126,-126,-127,-127,-127,-126,-126,-125,-123,-122,-120,-117,-115,-112,-109,-106,-102,-98,-94,-90,-85,-81,-76,-71,-65,-60,-54,-49,-43,-37,-31,-25,-19,-12,-6        },
        [synth_waveform_pulse_12p5] = {
                127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127
        },
        [synth_waveform_pulse_25] = {
                127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127
        },
        [synth_waveform_pulse_50] = {
                127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127
        },
        [synth_waveform_pulse_75] = {
                127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127
        },
        [synth_waveform_triangle] = {
                //0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,67,71,75,79,83,87,91,95,99,103,107,111,115,119,123,127,123,119,115,111,107,103,99,95,91,87,83,79,75,71,67,64,60,56,52,48,44,40,36,32,28,24,20,16,12,8,4,0,-4,-8,-12,-16,-20,-24,-28,-32,-36,-40,-44,-48,-52,-56,-60,-64,-67,-71,-75,-79,-83,-87,-91,-95,-99,-103,-107,-111,-115,-119,-123,-127,-123,-119,-115,-111,-107,-103,-99,-95,-91,-87,-83,-79,-75,-71,-67,-64,-60,-56,-52,-48,-44,-40,-36,-32,-28,-24,-20,-16,-12,-8,-4
                0,2,-9,-20,-17,-14,-25,-35,-33,-31,-41,-51,-49,-48,-58,-68,-67,-66,-74,-82,-82,-82,-90,-97,-99,-101,-108,-115,-118,-120,-124,-127,-127,-127,-124,-120,-118,-115,-108,-101,-99,-97,-90,-82,-82,-82,-74,-66,-67,-68,-58,-48,-49,-51,-41,-31,-33,-35,-25,-14,-17,-20,-9,2,0,-2,9,20,17,14,25,35,33,31,41,51,49,48,58,68,67,66,74,82,82,82,90,97,99,101,108,115,118,120,124,127,127,127,124,120,118,115,108,101,99,97,90,82,82,82,74,66,67,68,58,48,49,51,41,31,33,35,25,14,17,20,9,-2
        },
        [synth_waveform_sawtooth] = {
                0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,95,97,99,101,103,105,107,109,111,113,115,117,119,121,123,125,-127,-125,-123,-121,-119,-117,-115,-113,-111,-109,-107,-105,-103,-101,-99,-97,-95,-93,-91,-89,-87,-85,-83,-81,-79,-77,-75,-73,-71,-69,-67,-65,-64,-62,-60,-58,-56,-54,-52,-50,-48,-46,-44,-42,-40,-38,-36,-34,-32,-30,-28,-26,-24,-22,-20,-18,-16,-14,-12,-10,-8,-6,-4,-2
        },
        [synth_waveform_piano] = {
                7,11,8,-4,-12,-24,-25,-18,-7,12,23,40,51,73,83,103,107,107,97,67,50,25,19,20,29,49,69,75,81,86,100,107,100,88,51,35,12,4,-8,-13,-15,-14,-6,-3,0,-4,-18,-25,-32,-28,-21,-22,-25,-28,-24,-15,-11,-10,-9,-11,-12,-8,-5,5,7,4,-1,-10,-14,-13,-7,16,32,56,62,63,63,65,73,80,93,99,101,96,78,66,45,34,18,11,1,-3,-12,-17,-30,-38,-60,-71,-88,-93,-100,-104,-113,-119,-120,-125,-127,-124,-120,-104,-97,-83,-77,-65,-61,-58,-59,-62,-59,-53,-49,-44,-43,-41,-40,-33,-29,-15
        },
};
