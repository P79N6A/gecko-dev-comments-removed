







#include "sandbox/win/src/sidestep/mini_disassembler.h"
#include "sandbox/win/src/sidestep/mini_disassembler_types.h"

namespace sidestep {

const ModrmEntry MiniDisassembler::s_ia16_modrm_map_[] = {

   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { true, false, OS_WORD },
   { false, false, OS_ZERO },

   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },

   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },
   { true, false, OS_WORD },

   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO }
};

const ModrmEntry MiniDisassembler::s_ia32_modrm_map_[] = {

   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, true, OS_ZERO },
   { true, false, OS_DOUBLE_WORD },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },

   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, true, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },
   { true, false, OS_BYTE },

   { true, false, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },
   { true, true, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },
   { true, false, OS_DOUBLE_WORD },

   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
   { false, false, OS_ZERO },
};

};  
