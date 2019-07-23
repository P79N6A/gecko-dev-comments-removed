



































#ifndef nsILoggingSink_h___
#define nsILoggingSink_h___

#include "nsIHTMLContentSink.h"
#include "nsStringGlue.h"
#include "prprf.h"


#define NS_ILOGGING_SINK_IID \
 {0xa6cf9061, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsILoggingSink : public nsIHTMLContentSink {
public:
  NS_IMETHOD SetOutputStream(PRFileDesc *aStream,PRBool autoDelete=PR_FALSE) =0;
};

extern "C" nsresult NS_NewHTMLLoggingSink(nsIContentSink** aInstancePtrResult);

#endif 
