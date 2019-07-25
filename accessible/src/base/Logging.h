





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
class nsISelection;
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

  eEvents = 1 << 3,
  ePlatforms = 1 << 4,
  eStack = 1 << 5,
  eText = 1 << 6,
  eTree = 1 << 7,

  eDOMEvents = 1 << 8,
  eFocus = 1 << 9,
  eSelection = 1 << 10,
  eNotifications = eDOMEvents | eSelection | eFocus
};




bool IsEnabled(uint32_t aModules);




void DocLoad(const char* aMsg, nsIWebProgress* aWebProgress,
             nsIRequest* aRequest, uint32_t aStateFlags);
void DocLoad(const char* aMsg, nsIDocument* aDocumentNode);




void DocLoadEventFired(AccEvent* aEvent);




void DocLoadEventHandled(AccEvent* aEvent);




void DocCreate(const char* aMsg, nsIDocument* aDocumentNode,
               DocAccessible* aDocument = nullptr);




void DocDestroy(const char* aMsg, nsIDocument* aDocumentNode,
                DocAccessible* aDocument = nullptr);




void OuterDocDestroy(OuterDocAccessible* OuterDoc);




void SelChange(nsISelection* aSelection, DocAccessible* aDocument);






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

