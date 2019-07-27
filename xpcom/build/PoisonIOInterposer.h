





#ifndef mozilla_PoisonIOInterposer_h
#define mozilla_PoisonIOInterposer_h

#include "mozilla/Types.h"
#include <stdio.h>

MOZ_BEGIN_EXTERN_C


void MozillaRegisterDebugFD(int aFd);


void MozillaRegisterDebugFILE(FILE* aFile);


void MozillaUnRegisterDebugFD(int aFd);


void MozillaUnRegisterDebugFILE(FILE* aFile);

MOZ_END_EXTERN_C

#if defined(XP_WIN) || defined(XP_MACOSX)

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

#ifdef __cplusplus
namespace mozilla {
inline bool IsDebugFile(intptr_t aFileID) { return true; }
inline void InitPoisonIOInterposer() {}
inline void ClearPoisonIOInterposer() {}
#ifdef XP_MACOSX
inline void OnlyReportDirtyWrites() {}
#endif 
} 
#endif 

#endif 

#endif 
