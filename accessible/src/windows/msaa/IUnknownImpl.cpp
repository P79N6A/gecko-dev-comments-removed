






#include "IUnknownImpl.h"

#include "nsDebug.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

namespace mozilla {
namespace a11y {

HRESULT
GetHRESULT(nsresult aResult)
{
  switch (aResult) {
    case NS_OK:
      return S_OK;

    case NS_ERROR_INVALID_ARG: case NS_ERROR_INVALID_POINTER:
      return E_INVALIDARG;

    case NS_ERROR_OUT_OF_MEMORY:
      return E_OUTOFMEMORY;

    case NS_ERROR_NOT_IMPLEMENTED:
      return E_NOTIMPL;

    default:
      return E_FAIL;
  }
}

int
FilterExceptions(unsigned int aCode, EXCEPTION_POINTERS* aExceptionInfo)
{
  if (aCode == EXCEPTION_ACCESS_VIOLATION) {
#ifdef MOZ_CRASHREPORTER
    
    
    
    CrashReporter::WriteMinidumpForException(aExceptionInfo);
#endif
  } else {
    NS_NOTREACHED("We should only be catching crash exceptions");
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

} 
} 
