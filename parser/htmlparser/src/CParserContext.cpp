






































#include "nsIAtom.h"
#include "CParserContext.h"
#include "nsToken.h"
#include "prenv.h"  
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"

CParserContext::CParserContext(CParserContext* aPrevContext,
                               nsScanner* aScanner, 
                               void *aKey, 
                               eParserCommands aCommand,
                               nsIRequestObserver* aListener, 
                               eAutoDetectResult aStatus, 
                               PRBool aCopyUnused)
  : mListener(aListener),
    mKey(aKey),
    mPrevContext(aPrevContext),
    mScanner(aScanner),
    mDTDMode(eDTDMode_unknown),
    mStreamListenerState(eNone),
    mContextType(eCTNone),
    mAutoDetectStatus(aStatus),
    mParserCommand(aCommand),
    mMultipart(PR_TRUE),
    mCopyUnused(aCopyUnused),
    mNumConsumed(0)
{ 
  MOZ_COUNT_CTOR(CParserContext); 
} 

CParserContext::~CParserContext()
{
  
  MOZ_COUNT_DTOR(CParserContext);
}

void
CParserContext::SetMimeType(const nsACString& aMimeType)
{
  mMimeType.Assign(aMimeType);

  mDocType = ePlainText;

  if (mMimeType.EqualsLiteral(kHTMLTextContentType))
    mDocType = eHTML_Strict;
  else if (mMimeType.EqualsLiteral(kXMLTextContentType)          ||
           mMimeType.EqualsLiteral(kXMLApplicationContentType)   ||
           mMimeType.EqualsLiteral(kXHTMLApplicationContentType) ||
           mMimeType.EqualsLiteral(kXULTextContentType)          ||
#ifdef MOZ_SVG
           mMimeType.EqualsLiteral(kSVGTextContentType)          ||
#endif
           mMimeType.EqualsLiteral(kRDFApplicationContentType)   ||
           mMimeType.EqualsLiteral(kRDFTextContentType))
    mDocType = eXML;
}

nsresult
CParserContext::GetTokenizer(nsIDTD* aDTD, nsITokenizer*& aTokenizer)
{
  if (!mTokenizer)
    mTokenizer = aDTD->CreateTokenizer();
  return (aTokenizer = mTokenizer)
    ? NS_OK
    : NS_ERROR_OUT_OF_MEMORY;
}
