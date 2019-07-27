




#include "js/TypeDecls.h"
#include "nsGlobalWindow.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsString.h"
#include "xpcpublic.h"

namespace mozilla {

struct FeedWriterEnabled {
  static bool IsEnabled(JSContext* cx, JSObject* aGlobal)
  {
    
    nsGlobalWindow* win = xpc::WindowGlobalOrNull(aGlobal);
    if (!win) {
      return false;
    }

    
    nsCOMPtr<nsIPrincipal> principal = win->GetPrincipal();
    NS_ENSURE_TRUE(principal, false);
    nsCOMPtr<nsIURI> uri;
    principal->GetURI(getter_AddRefs(uri));
    if (!uri) {
      return false;
    }

    
    bool isAbout = false;
    uri->SchemeIs("about", &isAbout);
    if (!isAbout) {
      return false;
    }

    
    nsAutoCString spec;
    uri->GetSpec(spec);
    return spec.EqualsLiteral("about:feeds");
  }
};

}
