









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
#else
.macro GLOBAL_FUNCTION name
.global \name
.endm
.macro DEFINE_FUNCTION name
\name:
.endm
#endif

.text

#endif  
