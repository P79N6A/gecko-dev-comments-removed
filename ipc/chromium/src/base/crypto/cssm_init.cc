



#include "base/crypto/cssm_init.h"

#include <Security/cssm.h>

#include "base/logging.h"
#include "base/singleton.h"








namespace {

class CSSMInitSingleton {
 public:
  CSSMInitSingleton() : inited_(false), loaded_(false) {
    static CSSM_VERSION version = {2, 0};
    
    static const CSSM_GUID test_guid = {
      0xFADE, 0, 0, { 1, 2, 3, 4, 5, 6, 7, 0 }
    };
    CSSM_RETURN crtn;
    CSSM_PVC_MODE pvc_policy = CSSM_PVC_NONE;
    crtn = CSSM_Init(&version, CSSM_PRIVILEGE_SCOPE_NONE, &test_guid,
                     CSSM_KEY_HIERARCHY_NONE, &pvc_policy, NULL);
    if (crtn) {
      NOTREACHED();
      return;
    }
    inited_ = true;

    crtn = CSSM_ModuleLoad(&gGuidAppleCSP, CSSM_KEY_HIERARCHY_NONE, NULL, NULL);
    if (crtn) {
      NOTREACHED();
      return;
    }
    loaded_ = true;
  }

  ~CSSMInitSingleton() {
    CSSM_RETURN crtn;
    if (loaded_) {
      crtn = CSSM_ModuleUnload(&gGuidAppleCSP, NULL, NULL);
      DCHECK(crtn == CSSM_OK);
    }
    if (inited_) {
      crtn = CSSM_Terminate();
      DCHECK(crtn == CSSM_OK);
    }
  }

 private:
  bool inited_;  
  bool loaded_;  
};

}  

namespace base {

void EnsureCSSMInit() {
  Singleton<CSSMInitSingleton>::get();
}

}  
