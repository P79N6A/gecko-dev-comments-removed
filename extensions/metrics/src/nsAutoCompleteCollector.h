












































#ifndef nsAutoCompleteCollector_h_
#define nsAutoCompleteCollector_h_

#include "nsIMetricsCollector.h"
#include "nsIObserver.h"

class nsAutoCompleteCollector : public nsIMetricsCollector,
                                public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSCOLLECTOR
  NS_DECL_NSIOBSERVER

  nsAutoCompleteCollector();

 private:
  ~nsAutoCompleteCollector();
};

#define NS_AUTOCOMPLETECOLLECTOR_CLASSNAME "AutoComplete Collector"
#define NS_AUTOCOMPLETECOLLECTOR_CID \
{ 0x62cb877d, 0x5c8a, 0x44ca, {0xab, 0xcd, 0x1c, 0xaa, 0x76, 0x7c, 0xf4, 0xd4}}

#endif  
