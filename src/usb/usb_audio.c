/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Reinhard Panhuber
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xcore/hwtimer.h>
#include <dsp.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"

#include "usb_descriptors.h"
#include "tusb.h"

#include "rtos/drivers/intertile/api/rtos_intertile.h"

#include "midi_sequencer.h"
#include "synth/synth.h"
#include "app_conf.h"

// Audio controls
// Current states
bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];                      // +1 for master channel 0
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];                    // +1 for master channel 0
uint32_t sampFreq;
uint8_t clkValid;

// Range states
audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX+1];           // Volume range state
audio_control_range_4_n_t(1) sampleFreqRng;                         // Sample frequency range state

static volatile bool mic_interface_open = false;

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    rtos_printf("USB mounted\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    rtos_printf("USB unmounted\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    xassert(false);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{

}

//--------------------------------------------------------------------+
// AUDIO Task
//--------------------------------------------------------------------+

#if CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX == 1
typedef int8_t samp_t;
#elif CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX == 2
typedef int16_t samp_t;
#elif CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX == 4
typedef int32_t samp_t;
#else
#error CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX must be either 2 or 4
#endif

void usb_audio_in_task(void *arg)
{
    rtos_intertile_t *intertile_ctx = (rtos_intertile_t*) arg;

    while (!tusb_inited()) {
        vTaskDelay(10);
    }

    int note_number = 21;
    midi_sequencer_reset();

    for (;;) {

        if (mic_interface_open) {

            midi_sequencer_program_change(0);
            midi_sequencer_note_on(note_number, 127);
            midi_sequencer_program_change(10);
            midi_sequencer_note_on(note_number, 127);

            vTaskDelay(250);
            midi_sequencer_program_change(0);
            midi_sequencer_note_off(note_number, 127);
            midi_sequencer_program_change(10);
            midi_sequencer_note_off(note_number, 127);
            vTaskDelay(50);

            note_number++;
            if (note_number > 108) {
                note_number = 21;
            }
        } else {
            midi_sequencer_reset();
            note_number = 21;
        }
    }
}

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport,
                             tusb_control_request_t const *p_request,
                             uint8_t *pBuff)
{
    (void) rhport;
    (void) pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void) channelNum;
    (void) ctrlSel;
    (void) ep;

    return false;   // Yet not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_req_itf_cb(uint8_t rhport,
                              tusb_control_request_t const *p_request,
                              uint8_t *pBuff)
{
    (void) rhport;
    (void) pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void) channelNum;
    (void) ctrlSel;
    (void) itf;

    return false;   // Yet not implemented
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport,
                                 tusb_control_request_t const *p_request,
                                 uint8_t *pBuff)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    (void) itf;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // If request is for our feature unit
    if (entityID == UAC2_ENTITY_MIC_FEATURE_UNIT) {
        switch (ctrlSel) {
        case AUDIO_FU_CTRL_MUTE:
            // Request uses format layout 1
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));

            mute[channelNum] = ((audio_control_cur_1_t*) pBuff)->bCur;

            TU_LOG2("    Set Mute: %d of channel: %u\r\n", mute[channelNum], channelNum);

            return true;

        case AUDIO_FU_CTRL_VOLUME:
            // Request uses format layout 2
            TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_2_t));

            volume[channelNum] = ((audio_control_cur_2_t*) pBuff)->bCur;

            TU_LOG2("    Set Volume: %d dB of channel: %u\r\n", volume[channelNum], channelNum);

            return true;

            // Unknown/Unsupported control
        default:
            TU_BREAKPOINT();
            return false;
        }
    }
    return false;    // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport,
                             tusb_control_request_t const *p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void) channelNum;
    (void) ctrlSel;
    (void) ep;

    //  return tud_control_xfer(rhport, p_request, &tmp, 1);

    return false;   // Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport,
                              tusb_control_request_t const *p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void) channelNum;
    (void) ctrlSel;
    (void) itf;

    return false;   // Yet not implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport,
                                 tusb_control_request_t const *p_request)
{
    (void) rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    // uint8_t itf = TU_U16_LOW(p_request->wIndex);             // Since we have only one audio function implemented, we do not need the itf value
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // Input terminal (Microphone input)
    if (entityID == UAC2_ENTITY_MIC_INPUT_TERMINAL) {
        switch (ctrlSel) {
        case AUDIO_TE_CTRL_CONNECTOR:
            ;
            // The terminal connector control only has a get request with only the CUR attribute.

            audio_desc_channel_cluster_t ret;

            // Those are dummy values for now
            ret.bNrChannels = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
            ret.bmChannelConfig = 0;
            ret.iChannelNames = 0;

            TU_LOG2("    Get terminal connector\r\n");
            rtos_printf("Get terminal connector\r\n");

            return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));

            // Unknown/Unsupported control selector
        default:
            TU_BREAKPOINT();
            return false;
        }
    }

    // Feature unit
    if (entityID == UAC2_ENTITY_MIC_FEATURE_UNIT) {
        switch (ctrlSel) {
        case AUDIO_FU_CTRL_MUTE:
            // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
            // There does not exist a range parameter block for mute
            TU_LOG2("    Get Mute of channel: %u\r\n", channelNum);
            return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);

        case AUDIO_FU_CTRL_VOLUME:

            switch (p_request->bRequest) {
            case AUDIO_CS_REQ_CUR:
                TU_LOG2("    Get Volume of channel: %u\r\n", channelNum);
                return tud_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));
            case AUDIO_CS_REQ_RANGE:
                TU_LOG2("    Get Volume range of channel: %u\r\n", channelNum);

                // Copy values - only for testing - better is version below
                audio_control_range_2_n_t(1) ret;

                ret.wNumSubRanges = 1;
                ret.subrange[0].bMin = -90;     // -90 dB
                ret.subrange[0].bMax = 90;      // +90 dB
                ret.subrange[0].bRes = 1;       // 1 dB steps

                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void*) &ret, sizeof(ret));

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
            }

            // Unknown/Unsupported control
        default:
            TU_BREAKPOINT();
            return false;
        }
    }

    // Clock Source unit
    if (entityID == UAC2_ENTITY_CLOCK) {
        switch (ctrlSel) {
        case AUDIO_CS_CTRL_SAM_FREQ:

            // channelNum is always zero in this case

            switch (p_request->bRequest) {
            case AUDIO_CS_REQ_CUR:
                TU_LOG2("    Get Sample Freq.\r\n");
                return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));
            case AUDIO_CS_REQ_RANGE:
                TU_LOG2("    Get Sample Freq. range\r\n");
                //((tusb_control_request_t *)p_request)->wLength = 14;
                return tud_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
            }

        case AUDIO_CS_CTRL_CLK_VALID:
            // Only cur attribute exists for this request
            TU_LOG2("    Get Sample Freq. valid\r\n");
            return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));

            // Unknown/Unsupported control
        default:
            TU_BREAKPOINT();
            return false;
        }
    }

    TU_LOG2("  Unsupported entity: %d\r\n", entityID);
    return false;   // Yet not implemented
}

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport,
                                   uint8_t itf,
                                   uint8_t ep_in,
                                   uint8_t cur_alt_setting)
{
    static int ready;
    uint8_t buf[BYTES_PER_TX_FRAME_NOMINAL];

    (void) rhport;
    (void) itf;
    (void) ep_in;
    (void) cur_alt_setting;

    if (!mic_interface_open) {
        ready = 0;
        mic_interface_open = true;
    }

    samp_t samples[SAMPLES_PER_FRAME_NOMINAL][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX];
    for (int i = 0; i < SAMPLES_PER_FRAME_NOMINAL; i++) {
        for (int ch = 0; ch < CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX; ch++) {
            samples[i][ch] = sample_get_next(midi_sequencer_synth_state(), ch);
        }
    }
    memcpy(buf, samples, BYTES_PER_TX_FRAME_NOMINAL);

    /*
     * TODO:
     * WRITE BUFFER OUT TO A STREAM BUFFER.
     * THIS WILL BE PICKED UP BY A TASK DEFINED IN THIS FILE.
     * IT WILL SEND EACH BUFFER OUT OVER INTERTILE TX.
     * THE OTHER TILE WILL PICK IT UP AND SEND IT TO I2S.
     */

    tud_audio_write(buf, BYTES_PER_TX_FRAME_NOMINAL);

    return true;
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport,
                                    uint16_t n_bytes_copied,
                                    uint8_t itf,
                                    uint8_t ep_in,
                                    uint8_t cur_alt_setting)
{
    (void) rhport;
    (void) n_bytes_copied;
    (void) itf;
    (void) ep_in;
    (void) cur_alt_setting;

    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport,
                          tusb_control_request_t const *p_request)
{
    (void) rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

#if AUDIO_INPUT_ENABLED
    if (itf == ITF_NUM_AUDIO_STREAMING_MIC) {
        /* In case the interface is reset without
         * closing it first */
        mic_interface_open = false;
    }
#endif

    rtos_printf("Set audio interface %d alt %d\n", itf, alt);

    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport,
                                   tusb_control_request_t const *p_request)
{
    (void) rhport;
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

#if AUDIO_INPUT_ENABLED
    if (itf == ITF_NUM_AUDIO_STREAMING_MIC) {
        mic_interface_open = false;
    }
#endif

    rtos_printf("Close audio interface %d alt %d\n", itf, alt);

    return true;
}

void usb_audio_init(rtos_intertile_t *intertile_ctx,
                    unsigned priority)
{
    // Init values
    sampFreq = appconfAUDIO_PIPELINE_SAMPLE_RATE;
    clkValid = 1;

    sampleFreqRng.wNumSubRanges = 1;
    sampleFreqRng.subrange[0].bMin = appconfAUDIO_PIPELINE_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bMax = appconfAUDIO_PIPELINE_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bRes = 0;

    xTaskCreate((TaskFunction_t) usb_audio_in_task, "usb_audio_in_task", portTASK_STACK_DEPTH(usb_audio_in_task), intertile_ctx, priority, NULL);
}
