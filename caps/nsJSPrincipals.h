





#ifndef nsJSPrincipals_h__
#define nsJSPrincipals_h__
#include "jsapi.h"
#include "nsIPrincipal.h"

struct nsJSPrincipals : nsIPrincipal, JSPrincipals
{
  static bool Subsume(JSPrincipals *jsprin, JSPrincipals *other);
  static void Destroy(JSPrincipals *jsprin);

  



  static nsJSPrincipals* get(JSPrincipals *principals) {
    nsJSPrincipals *self = static_cast<nsJSPrincipals *>(principals);
    MOZ_ASSERT_IF(self, self->debugToken == DEBUG_TOKEN);
    return self;
  }
  
  static nsJSPrincipals* get(nsIPrincipal *principal) {
    nsJSPrincipals *self = static_cast<nsJSPrincipals *>(principal);
    MOZ_ASSERT_IF(self, self->debugToken == DEBUG_TOKEN);
    return self;
  }

  nsJSPrincipals() {
    refcount = 0;
    setDebugToken(DEBUG_TOKEN);
  }

  virtual ~nsJSPrincipals() {
    setDebugToken(0);
  }

  


  virtual void GetScriptLocation(nsACString &aStr) = 0;

#ifdef DEBUG
  virtual void dumpImpl() = 0;
#endif

  static const uint32_t DEBUG_TOKEN = 0x0bf41760;
};

#endif 
