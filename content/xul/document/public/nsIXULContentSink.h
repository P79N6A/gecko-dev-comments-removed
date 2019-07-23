





































#ifndef nsIXULContentSink_h__
#define nsIXULContentSink_h__

#include "nsIXMLContentSink.h"

class nsIDocument;
class nsXULPrototypeDocument;


#define NS_IXULCONTENTSINK_IID \
{ 0x26b87c63, 0x1b6b, 0x46af, \
  { 0xb6, 0x85, 0x11, 0xbb, 0xf3, 0x9a, 0xa8, 0x4a } }

class nsIXULContentSink : public nsIXMLContentSink
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXULCONTENTSINK_IID)

    




    NS_IMETHOD Init(nsIDocument* aDocument, nsXULPrototypeDocument* aPrototype) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXULContentSink, NS_IXULCONTENTSINK_IID)

nsresult
NS_NewXULContentSink(nsIXULContentSink** aResult);

#endif 
