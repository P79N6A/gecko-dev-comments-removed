





































#ifndef nsWindowCollector_h_
#define nsWindowCollector_h_

#include "nsIMetricsCollector.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"

class nsIDOMWindow;
class nsITimer;

































class nsWindowCollector : public nsIMetricsCollector,
                          public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSCOLLECTOR
  NS_DECL_NSIOBSERVER

  nsWindowCollector();

  
  void LogWindowOpen(nsITimer *timer, nsISupports *subject);

 private:
  ~nsWindowCollector();

  
  static void WindowOpenCallback(nsITimer *timer, void *closure);

  
  nsTArray< nsCOMPtr<nsITimer> > mWindowOpenTimers;
};

#define NS_WINDOWCOLLECTOR_CLASSNAME "Window Collector"
#define NS_WINDOWCOLLECTOR_CID \
{ 0x56e37604, 0xd593, 0x47e4, {0x87, 0x1f, 0x76, 0x13, 0x64, 0x8e, 0x74, 0x2b}}

#endif 
