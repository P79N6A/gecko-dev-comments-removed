











































#ifndef nsIRDFContentSink_h___
#define nsIRDFContentSink_h___

#include "nsIXMLContentSink.h"
class nsIDocument;
class nsIRDFDataSource;
class nsIURI;


#define NS_IRDFCONTENTSINK_IID \
{ 0x3a7459d7, 0xd723, 0x483c, { 0xae, 0xf0, 0x40, 0x4f, 0xc4, 0x8e, 0x09, 0xb8 } }





class nsIRDFContentSink : public nsIXMLContentSink {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IRDFCONTENTSINK_IID)

    


    NS_IMETHOD Init(nsIURI* aURL) = 0;

    


    NS_IMETHOD SetDataSource(nsIRDFDataSource* aDataSource) = 0;

    


    NS_IMETHOD GetDataSource(nsIRDFDataSource*& rDataSource) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIRDFContentSink, NS_IRDFCONTENTSINK_IID)





nsresult
NS_NewRDFContentSink(nsIRDFContentSink** aResult);

#endif 
