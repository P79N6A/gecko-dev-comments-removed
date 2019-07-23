






































#include "nsIPromptService.h"
#include "nsString.h"
#include <gtk/gtk.h>

class nsIDOMWindow;

class GtkPromptService : public nsIPromptService
{
public:
    GtkPromptService();
    virtual ~GtkPromptService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPTSERVICE

private:
    GtkWindow* GetGtkWindowForDOMWindow(nsIDOMWindow* aDOMWindow);
    void GetButtonLabel(PRUint32 aFlags, PRUint32 aPos,
                        const PRUnichar* aStringValue, nsAString &aLabel);
};
