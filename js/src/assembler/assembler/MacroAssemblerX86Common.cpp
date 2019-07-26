





#include "assembler/wtf/Platform.h"


#if WTF_CPU_X86 || WTF_CPU_X86_64

#include "assembler/assembler/MacroAssemblerX86Common.h"

using namespace JSC;
MacroAssemblerX86Common::SSECheckState MacroAssemblerX86Common::s_sseCheckState = NotCheckedSSE;

#ifdef DEBUG
bool MacroAssemblerX86Common::s_floatingPointDisabled = false;
bool MacroAssemblerX86Common::s_SSE3Disabled = false;
bool MacroAssemblerX86Common::s_SSE4Disabled = false;
#endif

#endif 

