






































#include "nsIAtom.h"
#include "CParserContext.h"
#include "nsToken.h"
#include "prenv.h"  
#include "nsIHTMLContentSink.h"
#include "nsHTMLTokenizer.h"
#include "nsMimeTypes.h"

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

  if (mMimeType.EqualsLiteral(TEXT_HTML))
    mDocType = eHTML_Strict;
  else if (mMimeType.EqualsLiteral(TEXT_XML)              ||
           mMimeType.EqualsLiteral(APPLICATION_XML)       ||
           mMimeType.EqualsLiteral(APPLICATION_XHTML_XML) ||
           mMimeType.EqualsLiteral(TEXT_XUL)              ||
           mMimeType.EqualsLiteral(IMAGE_SVG_XML)         ||
           mMimeType.EqualsLiteral(APPLICATION_MATHML_XML) ||
           mMimeType.EqualsLiteral(APPLICATION_RDF_XML)   ||
           mMimeType.EqualsLiteral(TEXT_RDF))
    mDocType = eXML;
}

nsresult
CParserContext::GetTokenizer(nsIDTD* aDTD,
                             nsIContentSink* aSink,
                             nsITokenizer*& aTokenizer)
{
  nsresult result = NS_OK;
  PRInt32 type = aDTD ? aDTD->GetType() : NS_IPARSER_FLAG_HTML;

  if (!mTokenizer) {
    if (type == NS_IPARSER_FLAG_HTML || mParserCommand == eViewSource) {
      nsCOMPtr<nsIHTMLContentSink> theSink = do_QueryInterface(aSink);
      mTokenizer = new nsHTMLTokenizer(mDTDMode, mDocType, mParserCommand,
                                       nsHTMLTokenizer::GetFlags(aSink));
      if (!mTokenizer) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      
      
      if (mPrevContext) {
        mTokenizer->CopyState(mPrevContext->mTokenizer);
      }
    }
    else if (type == NS_IPARSER_FLAG_XML) {
      mTokenizer = do_QueryInterface(aDTD, &result);
    }
  }

  aTokenizer = mTokenizer;

  return result;
}
