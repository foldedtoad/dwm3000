#include <string.h>
#include <mac_802_15_8.h>


/* @fn      rx_aes_802_15_8
 * @brief   Decrypts received frame, the frame type needs to match the structure defined in deca_device_api.h - dwt_test_aes_header_s.
 *          Note, register key AES128 should be set before first usage of a function.
 * @param   frame_length-length data that was received, aes_job-holds the params for decryption, payload-payload pointer,
 *          payload_load_size-size of receive buffer
 *
 * @return aes_results_e
 * */
aes_results_e rx_aes_802_15_8(uint16_t frame_length,dwt_aes_job_t *aes_job,uint8_t *payload,uint16_t payload_load_size,dwt_aes_core_type_e core_type)
{
    uint8_t    nonce[12];
    int8_t   status;
    int16_t  payload_len;

    mac_frame_802_15_8_format_t header = {0};

    payload_len = frame_length - (sizeof(header) +aes_job->mic_size +FCS_LEN); /* hard-coded MIC size of 16 bytes and 2 bytes of FCS */

    if (payload_len >= 0 && payload_len < payload_load_size)
    {
        /* Download a max size of a plain text header which we are expecting in the frame. */
        dwt_readrxdata((uint8_t *)&header, sizeof(header), 0);

        /* Place a breakpoint here to see an unencrypted header */

        /* Construct of a nonce to be used for decryption.
         * Nonce received as a part of a header plain text.
         * In 802.15.8 standard, the 96-bit nonce shall be constructed, using 6 bytes of Sender address and
         * 6 bytes of a unique part, called PN.
         * */
        memcpy(&nonce[0], &header.nonce[0], 6);
        memcpy(&nonce[6], &header.src_addr[0], 6);

        /* Fill AES job to decrypt the received packet */
        aes_job->nonce       = nonce;
        aes_job->header_len  = sizeof(header);
        aes_job->payload_len = payload_len;
        aes_job->header=NULL;
        aes_job->payload=payload;
        //dwt_configure_aes(aes_job);
        status = dwt_do_aes(aes_job,core_type);//After this command, payload will contain the received data

        /* "status" represents a last read of AES_STS_ID register.
        * See DW3000 User Manual for details.
        * */
        if (status<0)
        {//Problem with Header/Payload length or mode selection
            return AES_RES_ERROR_LENGTH;
        }
        else
        {
            if (status & AES_ERRORS)
            {//One of the error status bits were raised
                return AES_RES_ERROR;
            }
            else
            {
                return AES_RES_OK;
            }
        }
    }
    else
    {
        return AES_RES_ERROR_FRAME;
    }
}
