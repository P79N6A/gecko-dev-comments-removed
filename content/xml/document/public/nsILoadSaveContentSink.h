





































#ifndef nsILoadSaveContentSink_h__
#define nsILoadSaveContentSink_h__

#include "nsIXMLContentSink.h"

#define NS_ILOADSAVE_CONTENT_SINK_IID \
  { 0xa39ed66a, 0x6ef5, 0x49da, \
  { 0xb6, 0xe4, 0x9e, 0x15, 0x85, 0xf0, 0xba, 0xc9 } }






class nsILoadSaveContentSink : public nsIXMLContentSink {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILOADSAVE_CONTENT_SINK_IID)

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILoadSaveContentSink,
                              NS_ILOADSAVE_CONTENT_SINK_IID)






nsresult
NS_NewLoadSaveContentSink(nsILoadSaveContentSink** aResult,
                          nsIXMLContentSink* aBaseSink);

#endif
