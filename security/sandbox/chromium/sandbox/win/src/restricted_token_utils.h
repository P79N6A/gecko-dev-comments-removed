



#ifndef SANDBOX_SRC_RESTRICTED_TOKEN_UTILS_H__
#define SANDBOX_SRC_RESTRICTED_TOKEN_UTILS_H__

#include <accctrl.h>
#include <windows.h>

#include "sandbox/win/src/restricted_token.h"
#include "sandbox/win/src/security_level.h"




namespace sandbox {


enum TokenType {
  IMPERSONATION = 0,
  PRIMARY
};











DWORD CreateRestrictedToken(HANDLE *token_handle,
                            TokenLevel security_level,
                            IntegrityLevel integrity_level,
                            TokenType token_type);





















DWORD StartRestrictedProcessInJob(wchar_t *command_line,
                                  TokenLevel primary_level,
                                  TokenLevel impersonation_level,
                                  JobLevel job_level,
                                  HANDLE *job_handle);


DWORD SetObjectIntegrityLabel(HANDLE handle, SE_OBJECT_TYPE type,
                              const wchar_t* ace_access,
                              const wchar_t* integrity_level_sid);




DWORD SetTokenIntegrityLevel(HANDLE token, IntegrityLevel integrity_level);



const wchar_t* GetIntegrityLevelString(IntegrityLevel integrity_level);




DWORD SetProcessIntegrityLevel(IntegrityLevel integrity_level);

}  

#endif  
