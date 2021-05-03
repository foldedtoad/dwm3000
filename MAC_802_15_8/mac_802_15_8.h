#ifndef _802_15_8_
#define _802_15_8_

#ifdef __cplusplus
extern "C" {
#endif

#include <deca_regs.h>
#include <deca_device_api.h>
#include <shared_defines.h>

/* Frame will be sent in a 802.15.8 format with 48 bit addresses */
typedef struct
{
    uint8_t fc[2];
    uint8_t seq;
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint8_t nonce[6];
}mac_frame_802_15_8_format_t;


/* @fn      dwt_rx_aes_802_15_8
 * @brief   RX callback which decrypts received frame, which sent from corresponded "TX" example.
 *          This function assumes frame received with known Header, as per test_aes_header_s.
 *          Note, register key AES128 should be set before first usage of a function.
 * */
aes_results_e rx_aes_802_15_8(uint16_t frame_length,dwt_aes_job_t *aes_job,uint8_t *payload,uint16_t payload_load_size,dwt_aes_core_type_e core_type);


#ifdef __cplusplus
}
#endif


#endif
