





































#ifndef nsUICommandCollector_h_
#define nsUICommandCollector_h_

#include "nsIObserver.h"
#include "nsIDOMEventListener.h"
#include "nsIMetricsCollector.h"

#include "nsDataHashtable.h"

class nsIDOMWindow;
class nsIMetricsEventItem;
class nsIWritablePropertyBag2;

class nsUICommandCollector : public nsIObserver,
                             public nsIDOMEventListener,
                             public nsIMetricsCollector
{
 public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIMETRICSCOLLECTOR
  
  static PLDHashOperator PR_CALLBACK AddCommandEventListener(
    const nsIDOMWindow* key, PRUint32 windowID, void* userArg);

  static PLDHashOperator PR_CALLBACK RemoveCommandEventListener(
    const nsIDOMWindow* key, PRUint32 windowID, void* userArg);

  nsUICommandCollector();

  
  
  nsresult GetEventTargets(nsIDOMEvent *event,
                           nsString &targetId, nsString &targetAnonId) const;

  
  
  void GetEventKeyId(nsIDOMEvent *event, nsString &keyId) const;

  
  nsresult GetEventWindow(nsIDOMEvent *event, PRUint32 *window) const;

 private:
  
  
  struct EventHandler {
    const char* event;
    nsresult (nsUICommandCollector::* handler)(nsIDOMEvent*);
  };

  
  static const EventHandler kEvents[];

  ~nsUICommandCollector();

  
  void AddEventListeners(nsIDOMEventTarget* window);

  
  void RemoveEventListeners(nsIDOMEventTarget* window);

  
  nsresult HandleCommandEvent(nsIDOMEvent* event);

  
  nsresult HandleTabMoveEvent(nsIDOMEvent* event);

  
  
  nsresult LogBookmarkInfo(const nsString& id,
                           nsIMetricsEventItem* parentItem);

  
  nsresult SetHashedValue(nsIWritablePropertyBag2 *properties,
                          const nsString &propertyName,
                          const nsString &propertyValue) const;
};

#define NS_UICOMMANDCOLLECTOR_CLASSNAME "UI Command Collector"
#define NS_UICOMMANDCOLLECTOR_CID \
{ 0xcc2fedc9, 0x8b2e, 0x4e2c, {0x97, 0x07, 0xe2, 0xe5, 0x6b, 0xeb, 0x01, 0x85}}

#endif 
