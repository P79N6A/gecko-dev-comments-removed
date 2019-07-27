



#ifndef SANDBOX_SRC_ACL_H_
#define SANDBOX_SRC_ACL_H_

#include <windows.h>

#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/sid.h"

namespace sandbox {


bool GetDefaultDacl(HANDLE token,
                    scoped_ptr_malloc<TOKEN_DEFAULT_DACL>* default_dacl);




bool AddSidToDacl(const Sid& sid, ACL* old_dacl, ACCESS_MASK access,
                  ACL** new_dacl);



bool AddSidToDefaultDacl(HANDLE token, const Sid& sid, ACCESS_MASK access);



bool AddUserSidToDefaultDacl(HANDLE token, ACCESS_MASK access);



bool AddKnownSidToKernelObject(HANDLE object, const Sid& sid,
                               ACCESS_MASK access);

}  


#endif  
