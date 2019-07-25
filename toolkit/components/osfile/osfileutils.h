#ifndef mozilla_osfileutils_h__
#define mozilla_osfileutils_h__

#include "mozilla/Types.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"

extern "C" {






MOZ_EXPORT_API(void) osfile_ns_free(void* buf);












MOZ_EXPORT_API(PRUnichar*) osfile_wstrdup(PRUnichar* source);












MOZ_EXPORT_API(PRUnichar*) osfile_DecodeAll(
   const char* aEncoding,
   const char* aSource,
   const int32_t aBytesToDecode);













MOZ_EXPORT_API(char*) osfile_EncodeAll(
   const char* aEncoding,
   const PRUnichar* aSource,
   int32_t* aBytesWritten);

} 

#endif 
