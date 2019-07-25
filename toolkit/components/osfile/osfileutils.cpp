
#include "mozilla/Scoped.h"

#include "osfileutils.h"
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsCRTGlue.h"


namespace {

#if defined(XP_WIN)
#include <windows.h>





void error_not_supported() {
  SetLastError(ERROR_NOT_SUPPORTED);
}






void error_invalid_argument() {
  SetLastError(ERROR_INVALID_DATA);
}






void error_no_memory() {
  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
}

#else

#include "errno.h"





void error_not_supported() {
  errno = ENOTSUP;
}






void error_invalid_argument() {
  errno = EINVAL;
}






void error_no_memory() {
  errno = ENOMEM;
}

#endif 

}


extern "C" {



MOZ_EXPORT_API(void) osfile_ns_free(void* buf) {
  NS_Free(buf);
}



MOZ_EXPORT_API(PRUnichar*) osfile_wstrdup(PRUnichar* string) {
  return NS_strdup(string);
}

MOZ_EXPORT_API(PRUnichar*) osfile_DecodeAll(
   const char* aEncoding,
   const char* aSource,
   const int32_t aBytesToDecode)
{
  if (!aEncoding || !aSource) {
    error_invalid_argument();
    return nsnull;
  }

  nsresult rv;
  nsCOMPtr<nsICharsetConverterManager> manager =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    error_not_supported();
    return nsnull;
  }

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = manager->GetUnicodeDecoder(aEncoding, getter_AddRefs(decoder));
  if (NS_FAILED(rv)) {
    error_invalid_argument();
    return nsnull;
  }

  

  int32_t srcBytes = aBytesToDecode;
  int32_t upperBoundChars = 0;
  rv = decoder->GetMaxLength(aSource, srcBytes, &upperBoundChars);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  int32_t bufSize = (upperBoundChars + 1) * sizeof (PRUnichar);

  mozilla::ScopedFreePtr<PRUnichar> dest((PRUnichar*)NS_Alloc(bufSize));
  if (dest.get() == nsnull) {
    error_no_memory();
    return nsnull;
  }

  

  rv = decoder->Convert(aSource, &srcBytes, dest.rwget(), &upperBoundChars);
  if (NS_FAILED(rv)) {
    error_invalid_argument();
    return nsnull;
  }

  dest.rwget()[upperBoundChars] = '\0';

  return dest.forget();
}

MOZ_EXPORT_API(char*) osfile_EncodeAll(
   const char* aEncoding,
   const PRUnichar* aSource,
   int32_t* aBytesProduced)
{
  if (!aEncoding || !aSource || !aBytesProduced) {
    error_invalid_argument();
    return nsnull;
  }

  nsresult rv;
  nsCOMPtr<nsICharsetConverterManager> manager =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    error_not_supported();
    return nsnull;
  }

  nsCOMPtr<nsIUnicodeEncoder> encoder;
  rv = manager->GetUnicodeEncoder(aEncoding, getter_AddRefs(encoder));
  if (NS_FAILED(rv)) {
    error_invalid_argument();
    return nsnull;
  }

  int32_t srcChars = NS_strlen(aSource);

  int32_t upperBoundBytes = 0;
  rv = encoder->GetMaxLength(aSource, srcChars, &upperBoundBytes);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  printf_stderr("Encoding %d chars into at up to %d bytes\n", srcChars, upperBoundBytes);
  int32_t bufSize = upperBoundBytes;
  mozilla::ScopedFreePtr<char> dest((char*)NS_Alloc(bufSize));

  if (dest.get() == nsnull) {
    error_no_memory();
    return nsnull;
  }

  rv = encoder->Convert(aSource, &srcChars, dest.rwget(), &upperBoundBytes);
  if (NS_FAILED(rv)) {
    error_invalid_argument();
    return nsnull;
  }

  *aBytesProduced = upperBoundBytes;
  return dest.forget();
}

} 
