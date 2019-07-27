




























#ifndef assembler_assembler_MacroAssembler_h
#define assembler_assembler_MacroAssembler_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER

#if JS_CODEGEN_NONE

#include "jit/none/BaseMacroAssembler-none.h"
namespace JSC { typedef MacroAssemblerNone MacroAssembler; }

#elif JS_CODEGEN_ARM


#elif WTF_CPU_MIPS
#include "assembler/assembler/MacroAssemblerMIPS.h"
namespace JSC { typedef MacroAssemblerMIPS MacroAssembler; }

#elif WTF_CPU_X86
#include "assembler/assembler/MacroAssemblerX86.h"
namespace JSC { typedef MacroAssemblerX86 MacroAssembler; }

#elif WTF_CPU_X86_64
#include "assembler/assembler/MacroAssemblerX86_64.h"
namespace JSC { typedef MacroAssemblerX86_64 MacroAssembler; }

#elif WTF_CPU_SPARC
#include "assembler/assembler/MacroAssemblerSparc.h"
namespace JSC { typedef MacroAssemblerSparc MacroAssembler; }

#else
#error "The MacroAssembler is not supported on this platform."
#endif

#endif 

#endif 
