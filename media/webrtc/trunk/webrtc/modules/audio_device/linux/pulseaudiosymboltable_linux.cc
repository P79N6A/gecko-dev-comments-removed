


























#include "pulseaudiosymboltable_linux.h"

namespace webrtc_adm_linux_pulse {

#ifdef __OpenBSD__
LATE_BINDING_SYMBOL_TABLE_DEFINE_BEGIN(PulseAudioSymbolTable, "libpulse.so")
#else
LATE_BINDING_SYMBOL_TABLE_DEFINE_BEGIN(PulseAudioSymbolTable, "libpulse.so.0")
#endif
#define X(sym) \
    LATE_BINDING_SYMBOL_TABLE_DEFINE_ENTRY(PulseAudioSymbolTable, sym)
PULSE_AUDIO_SYMBOLS_LIST
#undef X
LATE_BINDING_SYMBOL_TABLE_DEFINE_END(PulseAudioSymbolTable)

}  
