/*
 * usb_midi.c
 *
 *  Created on: Jun 1, 2021
 *      Author: mbruno
 */

#include <rtos_printf.h>
#include <tusb.h>
#include "midi_sequencer.h"

void tud_midi_rx_cb(uint8_t itf)
{
    uint8_t event[4];

    while (tud_midi_n_packet_read(itf, event)) {

        uint8_t param1 = event[2] & 0x7F;
        uint8_t param2 = event[3] & 0x7F;

        switch (event[0] & 0xF) {
        case 0x8: /* note off */
            if (param2 == 0) {
                param2 = 0x7F;
            }
            midi_sequencer_note_off(param1, param2);
            break;

        case 0x9: /*note on */
            if ((event[3] & 0x7F) == 0) {
                midi_sequencer_note_off(param1, 0x7F);
            } else {
                midi_sequencer_note_on(param1, param2);
            }
            break;

        case 0xC: /* program change */
            midi_sequencer_program_change(param1);
            break;

        default:
            break;
        }
    }
}
