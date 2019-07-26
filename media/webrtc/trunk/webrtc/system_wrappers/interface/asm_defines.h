









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ASM_DEFINES_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ASM_DEFINES_H_

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif



#ifdef __APPLE__
.macro GLOBAL_FUNCTION name
.global _\name
.endm
.macro DEFINE_FUNCTION name
_\name:
.endm
.macro CALL_FUNCTION name
bl _\name
.endm
.macro GLOBAL_LABEL name
.global _\name
.endm
#else
.macro GLOBAL_FUNCTION name
.global \name
.endm
.macro DEFINE_FUNCTION name
\name:
.endm
.macro CALL_FUNCTION name
bl \name
.endm
.macro GLOBAL_LABEL name
.global \name
.endm
#endif




#if (defined __llvm__) && (defined __APPLE__)
.macro streqh reg1, reg2, num
strheq \reg1, \reg2, \num
.endm
#endif

.text

#endif  
