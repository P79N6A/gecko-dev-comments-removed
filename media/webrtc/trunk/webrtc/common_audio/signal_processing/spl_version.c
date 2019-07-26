
















#include <string.h>
#include "signal_processing_library.h"

int16_t WebRtcSpl_get_version(char* version, int16_t length_in_bytes)
{
    strncpy(version, "1.2.0", length_in_bytes);
    return 0;
}
