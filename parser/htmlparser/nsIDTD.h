




#ifndef nsIDTD_h___
#define nsIDTD_h___
















#include "nsISupports.h"
#include "nsString.h"
#include "nsITokenizer.h"

#define NS_IDTD_IID \
{ 0x3de05873, 0xefa7, 0x410d, \
  { 0xa4, 0x61, 0x80, 0x33, 0xaf, 0xd9, 0xe3, 0x26 } }

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


class nsIContentSink;
class CParserContext;

class nsIDTD : public nsISupports
{
public:

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDTD_IID)

    NS_IMETHOD WillBuildModel(const CParserContext& aParserContext,
                              nsITokenizer* aTokenizer,
                              nsIContentSink* aSink) = 0;

    





    NS_IMETHOD DidBuildModel(nsresult anErrorCode) = 0;

    










    NS_IMETHOD BuildModel(nsITokenizer* aTokenizer, nsIContentSink* aSink) = 0;

    








    NS_IMETHOD_(bool) CanContain(int32_t aParent,int32_t aChild) const = 0;

    







    NS_IMETHOD_(bool) IsContainer(int32_t aTag) const = 0;

    









    NS_IMETHOD_(void) Terminate() = 0;

    NS_IMETHOD_(int32_t) GetType() = 0;

    



    NS_IMETHOD_(nsDTDMode) GetMode() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDTD, NS_IDTD_IID)

#define NS_DECL_NSIDTD \
    NS_IMETHOD WillBuildModel(  const CParserContext& aParserContext, nsITokenizer* aTokenizer, nsIContentSink* aSink) override;\
    NS_IMETHOD DidBuildModel(nsresult anErrorCode) override;\
    NS_IMETHOD BuildModel(nsITokenizer* aTokenizer, nsIContentSink* aSink) override;\
    NS_IMETHOD_(bool) CanContain(int32_t aParent,int32_t aChild) const override;\
    NS_IMETHOD_(bool) IsContainer(int32_t aTag) const override;\
    NS_IMETHOD_(void)  Terminate() override;\
    NS_IMETHOD_(int32_t) GetType() override;\
    NS_IMETHOD_(nsDTDMode) GetMode() const override;
#endif 
