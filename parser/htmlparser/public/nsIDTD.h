




































#ifndef nsIDTD_h___
#define nsIDTD_h___
















#include "nsISupports.h"
#include "nsStringGlue.h"
#include "prtypes.h"
#include "nsITokenizer.h"

#define NS_IDTD_IID \
{ 0xcc374204, 0xcea2, 0x41a2, \
  { 0xb2, 0x7f, 0x83, 0x75, 0xe2, 0xcf, 0x97, 0xcf } }


enum eAutoDetectResult {
    eUnknownDetect,
    eValidDetect,
    ePrimaryDetect,
    eInvalidDetect
};

enum nsDTDMode {
    eDTDMode_unknown = 0,
    eDTDMode_quirks,        
    eDTDMode_almost_standards,
    eDTDMode_full_standards,
    eDTDMode_autodetect,
    eDTDMode_fragment
};


class nsIParser;
class CToken;
class nsIURI;
class nsIContentSink;
class CParserContext;

class nsIDTD : public nsISupports
{
public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDTD_IID)

    NS_IMETHOD WillBuildModel(const CParserContext& aParserContext,
                              nsITokenizer* aTokenizer,
                              nsIContentSink* aSink) = 0;

    





    NS_IMETHOD DidBuildModel(nsresult anErrorCode, PRBool aNotifySink,
                             nsIParser* aParser,
                             nsIContentSink* aSink) = 0;

    





    NS_IMETHOD BuildModel(nsIParser* aParser, nsITokenizer* aTokenizer,
                          nsITokenObserver* anObserver,
                          nsIContentSink* aSink) = 0;

    









    NS_IMETHOD HandleToken(CToken* aToken,nsIParser* aParser) = 0;

    





    NS_IMETHOD WillResumeParse(nsIContentSink* aSink) = 0;

    





    NS_IMETHOD WillInterruptParse(nsIContentSink* aSink) = 0;

    








    NS_IMETHOD_(PRBool) CanContain(PRInt32 aParent,PRInt32 aChild) const = 0;

    







    NS_IMETHOD_(PRBool) IsContainer(PRInt32 aTag) const = 0;

    









    NS_IMETHOD_(void) Terminate() = 0;

    NS_IMETHOD_(PRInt32) GetType() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDTD, NS_IDTD_IID)

#define NS_DECL_NSIDTD \
    NS_IMETHOD WillBuildModel(  const CParserContext& aParserContext, nsITokenizer* aTokenizer, nsIContentSink* aSink);\
    NS_IMETHOD DidBuildModel(nsresult anErrorCode,PRBool aNotifySink,nsIParser* aParser,nsIContentSink* aSink);\
    NS_IMETHOD BuildModel(nsIParser* aParser,nsITokenizer* aTokenizer,nsITokenObserver* anObserver,nsIContentSink* aSink);\
    NS_IMETHOD HandleToken(CToken* aToken,nsIParser* aParser);\
    NS_IMETHOD WillResumeParse(nsIContentSink* aSink = 0);\
    NS_IMETHOD WillInterruptParse(nsIContentSink* aSink = 0);\
    NS_IMETHOD_(PRBool) CanContain(PRInt32 aParent,PRInt32 aChild) const;\
    NS_IMETHOD_(PRBool) IsContainer(PRInt32 aTag) const;\
    NS_IMETHOD_(void)  Terminate();\
    NS_IMETHOD_(PRInt32) GetType();
#endif 
