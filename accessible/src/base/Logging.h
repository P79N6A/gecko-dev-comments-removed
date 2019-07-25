





#ifndef mozilla_a11y_logs_h__
#define mozilla_a11y_logs_h__

#include "nscore.h"
#include "nsAString.h"

class AccEvent;
class Accessible;
class DocAccessible;
class nsIDocument;
class nsIRequest;
class nsIWebProgress;

namespace mozilla {
namespace a11y {
namespace logging {

enum EModules {
  eDocLoad = 1 << 0,
  eDocCreate = 1 << 1,
  eDocDestroy = 1 << 2,
  eDocLifeCycle = eDocLoad | eDocCreate | eDocDestroy,
  ePlatforms = 1 << 3
};




bool IsEnabled(PRUint32 aModule);




void DocLoad(const char* aMsg, nsIWebProgress* aWebProgress,
             nsIRequest* aRequest, PRUint32 aStateFlags);
void DocLoad(const char* aMsg, nsIDocument* aDocumentNode);




void DocLoadEventFired(AccEvent* aEvent);




void DocLoadEventHandled(AccEvent* aEvent);




void DocCreate(const char* aMsg, nsIDocument* aDocumentNode,
               DocAccessible* aDocument = nsnull);




void DocDestroy(const char* aMsg, nsIDocument* aDocumentNode,
                DocAccessible* aDocument = nsnull);




void Msg(const char* aMsg);




void Text(const char* aText);




void Address(const char* aDescr, Accessible* aAcc);




void Stack();




void Enable(const nsAFlatCString& aModules);





void CheckEnv();

} 
} 
} 

#endif

