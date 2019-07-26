
















#ifndef insanity_pkix__pkixutil_h
#define insanity_pkix__pkixutil_h

#include "insanity/pkixtypes.h"
#include "prerror.h"
#include "seccomon.h"
#include "secerr.h"

namespace insanity { namespace pkix {

enum Result
{
  Success = 0,
  FatalError = -1,      
                        
  RecoverableError = -2 
                        
                        
};





inline Result
Fail(Result result, PRErrorCode errorCode)
{
  PR_ASSERT(result != Success);
  PR_SetError(errorCode, 0);
  return result;
}

inline Result
MapSECStatus(SECStatus srv)
{
  if (srv == SECSuccess) {
    return Success;
  }

  PRErrorCode error = PORT_GetError();
  switch (error) {
    case SEC_ERROR_EXTENSION_NOT_FOUND:
      return RecoverableError;

    case SEC_ERROR_LIBRARY_FAILURE:
    case SEC_ERROR_NO_MEMORY:
      return FatalError;
  }

  
  return RecoverableError;
}
} } 

#endif 
