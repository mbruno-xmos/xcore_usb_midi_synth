/*
 * synth_wave_table.c
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#include "synth.h"

const int8_t synth_wave_table[][SYNTH_WAVE_TABLE_SIZE] = {
        [synth_instrument_sine] = {
                1,7,13,19,26,32,38,44,50,56,61,67,72,77,82,87,92,96,100,104,107,111,114,117,119,121,123,125,126,127,127,127,127,127,127,127,126,124,122,120,118,115,112,109,106,102,98,94,89,85,80,75,69,64,58,53,47,41,35,29,23,16,10,4,-3,-9,-15,-22,-28,-34,-40,-46,-52,-57,-63,-68,-74,-79,-84,-88,-93,-97,-101,-105,-108,-111,-114,-117,-119,-121,-123,-125,-126,-127,-127,-127,-127,-127,-126,-125,-124,-122,-120,-118,-116,-113,-110,-106,-103,-99,-95,-91,-86,-81,-76,-71,-66,-60,-55,-49,-43,-37,-31,-25,-18,-12,-6,0
        },
        [synth_instrument_piano] = {
                7,11,8,-4,-12,-24,-25,-18,-7,12,23,40,51,73,83,103,107,107,97,67,50,25,19,20,29,49,69,75,81,86,100,107,100,88,51,35,12,4,-8,-13,-15,-14,-6,-3,0,-4,-18,-25,-32,-28,-21,-22,-25,-28,-24,-15,-11,-10,-9,-11,-12,-8,-5,5,7,4,-1,-10,-14,-13,-7,16,32,56,62,63,63,65,73,80,93,99,101,96,78,66,45,34,18,11,1,-3,-12,-17,-30,-38,-60,-71,-88,-93,-100,-104,-113,-119,-120,-125,-127,-124,-120,-104,-97,-83,-77,-65,-61,-58,-59,-62,-59,-53,-49,-44,-43,-41,-40,-33,-29,-15
        },
};
