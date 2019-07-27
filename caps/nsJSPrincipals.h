





#ifndef nsJSPrincipals_h__
#define nsJSPrincipals_h__
#include "jsapi.h"
#include "nsIPrincipal.h"

class nsJSPrincipals : public nsIPrincipal, public JSPrincipals
{
public:
  
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

  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;

  nsJSPrincipals() {
    refcount = 0;
    setDebugToken(DEBUG_TOKEN);
  }

  


  virtual void GetScriptLocation(nsACString &aStr) = 0;
  static const uint32_t DEBUG_TOKEN = 0x0bf41760;

protected:
  virtual ~nsJSPrincipals() {
    setDebugToken(0);
  }

};

#endif 
