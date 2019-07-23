







































#include "nsIPromptService.h"
#include "nsICookiePromptService.h"
#include "nsICookie.h"
#include "nsNetCID.h"
#include <gtk/gtk.h>
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif
#define NS_PROMPTSERVICE_CID \
 {0x95611356, 0xf583, 0x46f5, {0x81, 0xff, 0x4b, 0x3e, 0x01, 0x62, 0xc6, 0x19}}

class nsIDOMWindow;

class GtkPromptService : public nsIPromptService,
                         public nsICookiePromptService
{
public:
    GtkPromptService();
    virtual ~GtkPromptService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPTSERVICE
    NS_DECL_NSICOOKIEPROMPTSERVICE

#ifndef MOZ_NO_GECKO_UI_FALLBACK_1_8_COMPAT
private:
    void GetButtonLabel(PRUint32 aFlags, PRUint32 aPos,
                        const PRUnichar* aStringValue, nsAString &aLabel);
#endif
};
