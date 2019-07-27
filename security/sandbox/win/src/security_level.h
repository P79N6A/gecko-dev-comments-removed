



#ifndef SANDBOX_SRC_SECURITY_LEVEL_H_
#define SANDBOX_SRC_SECURITY_LEVEL_H_

#include "base/basictypes.h"

namespace sandbox {




enum IntegrityLevel {
  INTEGRITY_LEVEL_SYSTEM,
  INTEGRITY_LEVEL_HIGH,
  INTEGRITY_LEVEL_MEDIUM,
  INTEGRITY_LEVEL_MEDIUM_LOW,
  INTEGRITY_LEVEL_LOW,
  INTEGRITY_LEVEL_BELOW_LOW,
  INTEGRITY_LEVEL_UNTRUSTED,
  INTEGRITY_LEVEL_LAST
};















































enum TokenLevel {
   USER_LOCKDOWN = 0,
   USER_RESTRICTED,
   USER_LIMITED,
   USER_INTERACTIVE,
   USER_NON_ADMIN,
   USER_RESTRICTED_SAME_ACCESS,
   USER_UNPROTECTED
};









































enum JobLevel {
  JOB_LOCKDOWN = 0,
  JOB_RESTRICTED,
  JOB_LIMITED_USER,
  JOB_INTERACTIVE,
  JOB_UNPROTECTED,
  JOB_NONE
};










typedef uint64 MitigationFlags;



const MitigationFlags MITIGATION_DEP                              = 0x00000001;




const MitigationFlags MITIGATION_DEP_NO_ATL_THUNK                 = 0x00000002;




const MitigationFlags MITIGATION_SEHOP                            = 0x00000004;



const MitigationFlags MITIGATION_RELOCATE_IMAGE                   = 0x00000008;



const MitigationFlags MITIGATION_RELOCATE_IMAGE_REQUIRED          = 0x00000010;



const MitigationFlags MITIGATION_HEAP_TERMINATE                   = 0x00000020;





const MitigationFlags MITIGATION_BOTTOM_UP_ASLR                   = 0x00000040;





const MitigationFlags MITIGATION_HIGH_ENTROPY_ASLR                = 0x00000080;




const MitigationFlags MITIGATION_STRICT_HANDLE_CHECKS             = 0x00000100;




const MitigationFlags MITIGATION_WIN32K_DISABLE                   = 0x00000200;




const MitigationFlags MITIGATION_EXTENSION_DLL_DISABLE            = 0x00000400;





const MitigationFlags MITIGATION_DLL_SEARCH_ORDER        = 0x00000001ULL << 32;

}  

#endif  
