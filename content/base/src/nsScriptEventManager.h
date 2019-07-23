











































#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsVoidArray.h"

#include "nsIScriptEventManager.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"

class nsScriptEventManager : public nsIScriptEventManager
{


public:
  nsScriptEventManager(nsIDOMDocument *aDocument);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTEVENTMANAGER

protected:
  virtual ~nsScriptEventManager();

private:
  nsCOMPtr<nsIDOMNodeList> mScriptElements;
};
