



































#ifndef nsIRobotSink_h___
#define nsIRobotSink_h___

#include "nsIHTMLContentSink.h"
class nsIURI;
class nsIRobotSinkObserver;


#define NS_IROBOTSINK_IID     \
{ 0x61256800, 0xcfd8, 0x11d1, \
  {0x93, 0x28, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

class nsIRobotSink : public nsIHTMLContentSink {
public:
  NS_IMETHOD Init(nsIURI* aDocumentURL) = 0;
  NS_IMETHOD AddObserver(nsIRobotSinkObserver* aObserver) = 0;
  NS_IMETHOD RemoveObserver(nsIRobotSinkObserver* aObserver) = 0;

};

extern nsresult NS_NewRobotSink(nsIRobotSink** aInstancePtrResult);

#endif 
