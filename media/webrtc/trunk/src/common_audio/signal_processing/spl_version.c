
















#include <string.h>
#include "signal_processing_library.h"

WebRtc_Word16 WebRtcSpl_get_version(char* version, WebRtc_Word16 length_in_bytes)
{
    strncpy(version, "1.2.0", length_in_bytes);
    return 0;
}
