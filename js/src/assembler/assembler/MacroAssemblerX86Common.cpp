






































#include "assembler/wtf/Platform.h"


#if WTF_CPU_X86 || WTF_CPU_X86_64

#include "MacroAssemblerX86Common.h"

using namespace JSC;
MacroAssemblerX86Common::SSECheckState MacroAssemblerX86Common::s_sseCheckState = NotCheckedSSE;

#endif 

