




































#ifndef qtpromptservice_h
#define qtpromptservice_h

#include <nsIPromptService.h>
#include <nsString.h>
#include <qwidget.h>

class nsIDOMWindow;

class QtPromptService : public nsIPromptService
{
public:
    QtPromptService();
    virtual ~QtPromptService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROMPTSERVICE

private:
    QWidget* GetQWidgetForDOMWindow(nsIDOMWindow* aDOMWindow);
    QString GetButtonLabel(PRUint32 aFlags, PRUint32 aPos,
                           const PRUnichar* aStringValue);
};

#endif 
