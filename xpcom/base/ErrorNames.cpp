





#include "mozilla/ArrayUtils.h"
#include "mozilla/ErrorNames.h"
#include "nsString.h"
#include "prerror.h"

namespace {

struct ErrorEntry
{
  nsresult value;
  const char * name;
};

#undef ERROR
#define ERROR(key, val) {key, #key}

const ErrorEntry errors[] = {
  #include "ErrorList.h"
};

#undef ERROR

} 

namespace mozilla {

void
GetErrorName(nsresult rv, nsACString& name)
{
  for (size_t i = 0; i < ArrayLength(errors); ++i) {
    if (errors[i].value == rv) {
      name.AssignASCII(errors[i].name);
      return;
    }
  }

  bool isSecurityError = NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_SECURITY;

  
  
  
  
  MOZ_ASSERT(isSecurityError);

  name.AssignASCII(NS_SUCCEEDED(rv) ? "NS_ERROR_GENERATE_SUCCESS("
                                    : "NS_ERROR_GENERATE_FAILURE(");

  if (isSecurityError) {
    name.AppendASCII("NS_ERROR_MODULE_SECURITY");
  } else {
    
    
    name.AppendInt(NS_ERROR_GET_MODULE(rv));
  }

  name.AppendASCII(", ");

  const char * nsprName = nullptr;
  if (isSecurityError) {
    
    PRErrorCode nsprCode
      = -1 * static_cast<PRErrorCode>(NS_ERROR_GET_CODE(rv));
    nsprName = PR_ErrorToName(nsprCode);

    
    MOZ_ASSERT(nsprName);
  }

  if (nsprName) {
    name.AppendASCII(nsprName);
  } else {
    name.AppendInt(NS_ERROR_GET_CODE(rv));
  }

  name.AppendASCII(")");
}

} 
