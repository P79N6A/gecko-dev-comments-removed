





#ifndef mozilla_a11y_logs_h__
#define mozilla_a11y_logs_h__

#include "nscore.h"
#include "nsAString.h"

class AccEvent;
class Accessible;
class DocAccessible;

class nsIDocument;
class nsINode;
class nsIRequest;
class nsIWebProgress;

namespace mozilla {
namespace a11y {

class OuterDocAccessible;

namespace logging {

enum EModules {
  eDocLoad = 1 << 0,
  eDocCreate = 1 << 1,
  eDocDestroy = 1 << 2,
  eDocLifeCycle = eDocLoad | eDocCreate | eDocDestroy,
  ePlatforms = 1 << 3,
  eStack = 1 << 4,
  eText = 1 << 5,
  eTree = 1 << 6
};




bool IsEnabled(PRUint32 aModules);




void DocLoad(const char* aMsg, nsIWebProgress* aWebProgress,
             nsIRequest* aRequest, PRUint32 aStateFlags);
void DocLoad(const char* aMsg, nsIDocument* aDocumentNode);




void DocLoadEventFired(AccEvent* aEvent);




void DocLoadEventHandled(AccEvent* aEvent);




void DocCreate(const char* aMsg, nsIDocument* aDocumentNode,
               DocAccessible* aDocument = nsnull);




void DocDestroy(const char* aMsg, nsIDocument* aDocumentNode,
                DocAccessible* aDocument = nsnull);




void OuterDocDestroy(OuterDocAccessible* OuterDoc);






void MsgBegin(const char* aTitle, const char* aMsgText, ...);
void MsgEnd();




void MsgEntry(const char* aEntryText, ...);




void Text(const char* aText);




void Address(const char* aDescr, Accessible* aAcc);




void Node(const char* aDescr, nsINode* aNode);




void Stack();




void Enable(const nsAFlatCString& aModules);





void CheckEnv();

} 
} 
} 

#endif

