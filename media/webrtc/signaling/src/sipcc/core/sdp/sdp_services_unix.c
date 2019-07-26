






































#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"










void sdp_log_errmsg (sdp_errmsg_e errmsg, char *str)
{
    switch (errmsg) {

    case SDP_ERR_INVALID_CONF_PTR:
        SDP_ERROR("\nSDP: Invalid Config pointer (%s).", str);
        break;

    case SDP_ERR_INVALID_SDP_PTR:
        SDP_ERROR("\nSDP: Invalid SDP pointer (%s).", str);
        break;

    case SDP_ERR_INTERNAL:
        SDP_ERROR("\nSDP: Internal error (%s).", str);
        break;

    default:
        break;
    }
}








void sdp_dump_buffer (char * _ptr, int _size_bytes)
{
    buginf_msg(_ptr);
}












