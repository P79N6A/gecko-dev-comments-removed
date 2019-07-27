



#ifndef SANDBOX_SRC_ACL_H_
#define SANDBOX_SRC_ACL_H_

#include <AccCtrl.h>
#include <windows.h>

#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/sid.h"

namespace sandbox {


bool GetDefaultDacl(
    HANDLE token,
    scoped_ptr<TOKEN_DEFAULT_DACL, base::FreeDeleter>* default_dacl);




bool AddSidToDacl(const Sid& sid, ACL* old_dacl, ACCESS_MODE access_mode,
                  ACCESS_MASK access, ACL** new_dacl);



bool AddSidToDefaultDacl(HANDLE token, const Sid& sid, ACCESS_MASK access);



bool AddUserSidToDefaultDacl(HANDLE token, ACCESS_MASK access);



bool AddKnownSidToObject(HANDLE object, SE_OBJECT_TYPE object_type,
                         const Sid& sid, ACCESS_MODE access_mode,
                         ACCESS_MASK access);

}  


#endif  
