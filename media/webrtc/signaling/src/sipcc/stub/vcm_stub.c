





















































#include "cpr_types.h"
#include "vcm.h"
#include "rtp_defs.h"
#include "ccsdp.h"






void vcmInit()
{
    return ;
}



















short vcmRxOpen(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id,  cc_call_handle_t call_handle,
                  uint16_t port_requested, cpr_ip_addr_t *listen_ip,
                  boolean is_multicast, int *port_allocated)
{
    return 0;
}












short vcmTxOpen(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id, cc_call_handle_t call_handle)
{
    return 0;
}















void vcmRxAllocPort(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id,  cc_call_handle_t call_handle,
                       uint16_t port_requested, int *port_allocated)
{
    return;
}












void vcmRxReleasePort(cc_mcapid_t mcap_id, cc_groupid_t group_id,cc_streamid_t stream_id,  cc_call_handle_t call_handle, int port)
{
    return;
}

























int vcmRxStart(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id, cc_call_handle_t call_handle,
                 vcm_media_payload_type_t payload, cpr_ip_addr_t *local_addr, uint16_t port,
                   vcm_crypto_algorithmID algorithmID,
                   vcm_crypto_key_t *rx_key, vcm_mediaAttrs_t *attrs)
{
    return 0;
}




























int vcmTxStart(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id,  cc_call_handle_t call_handle,
                   vcm_media_payload_type_t payload, short tos, cpr_ip_addr_t *local_addr, uint16_t local_port, cpr_ip_addr_t *remote_ip_addr,
                   uint16_t remote_port,
                   vcm_crypto_algorithmID algorithmID,
                   vcm_crypto_key_t *tx_key,
                   vcm_mediaAttrs_t *attrs)
{
    return 0;
}













void vcmRxClose(cc_mcapid_t mcap_id, cc_groupid_t group_id,cc_streamid_t stream_id,  cc_call_handle_t call_handle)
{
    return;
}












void vcmTxClose(cc_mcapid_t mcap_id, cc_groupid_t group_id, cc_streamid_t stream_id, cc_call_handle_t call_handleS)
{
    return;
}











void vcmEnableSidetone(uint16_t side_tone)
{
    return;
}
















void vcmToneStart(vcm_tones_t tone, short alert_info, cc_call_handle_t call_handle, cc_groupid_t group_id,
                    cc_streamid_t stream_id, uint16_t direction)
{
    return;
}















void vcmToneStartWithSpeakerAsBackup(vcm_tones_t tone, short alert_info, cc_call_handle_t call_handle, cc_groupid_t group_id,
                    cc_streamid_t stream_id, uint16_t direction)
{
    return;
}
















void vcmToneStop(vcm_tones_t tone, cc_groupid_t group_id, cc_streamid_t cc_stream_id, cc_call_handle_t call_handle)
{
    return;
}












void vcmControlRinger(vcm_ring_mode_t ringMode, short once,
                        boolean alert_info, int line, cc_callid_t call_id)
{
    return;
}










void vcmSetSpeakerMode(boolean state)
{
    return;
}






int vcmGetAudioCodecList(int request_type)
{
    return 0;
}





int vcmGetVideoCodecList(int request_type)
{
    return 0;
}




int vcmGetVideoMaxSupportedPacketizationMode()
{
    return 0;
}













int vcmGetRtpStats(cc_mcapid_t mcap_id, cc_groupid_t group_id,
                      cc_streamid_t stream_id, cc_call_handle_t call_handle,
                      char *rx_stats, char *tx_stats)
{
    return 0;
}















boolean vcmAllocateBandwidth(cc_call_handle_t call_handle, int sessions)
{
    return TRUE;
}









void vcmRemoveBandwidth(cc_call_handle_t call_handle)
{
    return;
}










void vcmActivateWlan(boolean is_active)
{
    return;
}








void vcmFreeMediaPtr(void *ptr)
{
    return;
}












void vcmMediaControl(cc_call_handle_t call_handle, vcm_media_control_to_encoder_t to_encoder)
{
    return;
}










void vcmSetRtcpDscp(cc_groupid_t group_id, int dscp)
{
    return;
}



















boolean vcmCheckAttribs(uint32_t media_type, void *sdp_p, int level, void **rcapptr)
{
    return TRUE;
}

















void vcmPopulateAttribs(void *sdp_p, int level, uint32_t media_type,
                          uint16_t payload_number, boolean isOffer)
{
    return;
}












int vcmDtmfBurst(int digit, int duration, int direction)
{
    return 0;
}

int vcmGetILBCMode()
{
    return SIPSDP_ILBC_MODE20;
}

