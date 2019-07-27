




#include "xpcprivate.h"
#include "nsString.h"
#include "nsIObjectOutputStream.h"
#include "nsIObjectInputStream.h"
#include "nsJSPrincipals.h"
#include "plstr.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsStringBuffer.h"


#include "mozilla/dom/workers/Workers.h"

NS_IMETHODIMP_(MozExternalRefCountType)
nsJSPrincipals::AddRef()
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_PRECONDITION(int32_t(refcount) >= 0, "illegal refcnt");
  nsrefcnt count = ++refcount;
  NS_LOG_ADDREF(this, count, "nsJSPrincipals", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsJSPrincipals::Release()
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_PRECONDITION(0 != refcount, "dup release");
  nsrefcnt count = --refcount;
  NS_LOG_RELEASE(this, count, "nsJSPrincipals");
  if (count == 0) {
    delete this;
  }

  return count;
}

 bool
nsJSPrincipals::Subsume(JSPrincipals *jsprin, JSPrincipals *other)
{
    bool result;
    nsresult rv = nsJSPrincipals::get(jsprin)->Subsumes(nsJSPrincipals::get(other), &result);
    return NS_SUCCEEDED(rv) && result;
}

 void
nsJSPrincipals::Destroy(JSPrincipals *jsprin)
{
    
    
    

    nsJSPrincipals *nsjsprin = nsJSPrincipals::get(jsprin);

    
    

#ifdef NS_BUILD_REFCNT_LOGGING
    
    
    
    nsjsprin->refcount++;
    nsjsprin->AddRef();
    nsjsprin->refcount--;
#else
    nsjsprin->refcount++;
#endif
    nsjsprin->Release();
}

#ifdef DEBUG


JS_EXPORT_API(void)
JSPrincipals::dump()
{
    if (debugToken == nsJSPrincipals::DEBUG_TOKEN) {
      nsAutoCString str;
      static_cast<nsJSPrincipals *>(this)->GetScriptLocation(str);
      fprintf(stderr, "nsIPrincipal (%p) = %s\n", static_cast<void*>(this), str.get());
    } else if (debugToken == mozilla::dom::workers::kJSPrincipalsDebugToken) {
        fprintf(stderr, "Web Worker principal singleton (%p)\n", this);
    } else {
        fprintf(stderr,
                "!!! JSPrincipals (%p) is not nsJSPrincipals instance - bad token: "
                "actual=0x%x expected=0x%x\n",
                this, unsigned(debugToken), unsigned(nsJSPrincipals::DEBUG_TOKEN));
    }
}

#endif 
