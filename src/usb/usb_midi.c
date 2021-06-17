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

        uint8_t usb_cin = event[0] & 0x0F;
        uint8_t status =  event[1] >> 4;
        uint8_t channel = event[1] & 0x0F;
        uint8_t param1 =  event[2] & 0x7F;
        uint8_t param2 =  event[3] & 0x7F;

        switch (usb_cin) {
        case 0x8: /* note off */
            if (param2 == 0) {
                param2 = 0x7F;
            }
            midi_sequencer_note_off(channel, param1, param2);
            break;

        case 0x9: /*note on */
            if ((event[3] & 0x7F) == 0) {
                midi_sequencer_note_off(channel, param1, 0x7F);
            } else {
                midi_sequencer_note_on(channel, param1, param2);
            }
            break;

        case 0xB: /* control change */
            rtos_printf("Control change %d, %d for channel %d\n", param1, param2, channel);
            break;

        case 0xC: /* program change */
            rtos_printf("Program %d for channel %d\n", param1, channel);
            midi_sequencer_program_change(channel, param1);
            break;

        case 0xE: /* pitch change */
            midi_sequencer_pitch_change(channel, (((int) param2) << 7) | param1);
            break;

        default:
            break;
        }
    }
}
