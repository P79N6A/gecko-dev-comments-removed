


























#include "alsasymboltable_linux.h"

namespace webrtc_adm_linux_alsa {

LATE_BINDING_SYMBOL_TABLE_DEFINE_BEGIN(AlsaSymbolTable, "libasound.so.2")
#define X(sym) \
    LATE_BINDING_SYMBOL_TABLE_DEFINE_ENTRY(AlsaSymbolTable, sym)
ALSA_SYMBOLS_LIST
#undef X
LATE_BINDING_SYMBOL_TABLE_DEFINE_END(AlsaSymbolTable)

}  
