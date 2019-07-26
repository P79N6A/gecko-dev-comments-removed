





#ifndef mozilla_PoisonIOInterposer_h
#define mozilla_PoisonIOInterposer_h

#include "mozilla/Types.h"
#include <stdio.h>

#if defined(MOZ_ENABLE_PROFILER_SPS) && (defined(XP_WIN) || defined(XP_MACOSX))

MOZ_BEGIN_EXTERN_C


void MozillaRegisterDebugFD(int fd);


void MozillaRegisterDebugFILE(FILE *f);


void MozillaUnRegisterDebugFD(int fd);


void MozillaUnRegisterDebugFILE(FILE *f);

MOZ_END_EXTERN_C

#ifdef __cplusplus
namespace mozilla {




bool IsDebugFile(intptr_t aFileID);







void InitPoisonIOInterposer();

#ifdef XP_MACOSX






void OnlyReportDirtyWrites();
#endif 





void ClearPoisonIOInterposer();

} 
#endif 

#else 

MOZ_BEGIN_EXTERN_C
inline void MozillaRegisterDebugFD(int fd){}
inline void MozillaRegisterDebugFILE(FILE *f){}
inline void MozillaUnRegisterDebugFD(int fd){}
inline void MozillaUnRegisterDebugFILE(FILE *f){}
MOZ_END_EXTERN_C

#ifdef __cplusplus
namespace mozilla {
inline bool IsDebugFile(intptr_t aFileID){ return true; }
inline void InitPoisonIOInterposer(){}
inline void ClearPoisonIOInterposer(){}
#ifdef XP_MACOSX
inline void OnlyReportDirtyWrites(){}
#endif 
} 
#endif 

#endif 

#endif 
