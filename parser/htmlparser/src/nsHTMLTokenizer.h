











#ifndef __NSHTMLTOKENIZER
#define __NSHTMLTOKENIZER

#include "nsISupports.h"
#include "nsITokenizer.h"
#include "nsIDTD.h"
#include "prtypes.h"
#include "nsDeque.h"
#include "nsScanner.h"
#include "nsHTMLTokens.h"
#include "nsDTDUtils.h"





#ifdef _MSC_VER
#pragma warning( disable : 4275 )
#endif

class nsHTMLTokenizer : public nsITokenizer {
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSITOKENIZER
  nsHTMLTokenizer(nsDTDMode aParseMode = eDTDMode_quirks,
                  eParserDocType aDocType = eHTML_Quirks,
                  eParserCommands aCommand = eViewNormal,
                  uint32_t aFlags = 0);
  virtual ~nsHTMLTokenizer();

  static uint32_t GetFlags(const nsIContentSink* aSink);

protected:

  nsresult ConsumeTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner,bool& aFlushTokens);
  nsresult ConsumeStartTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner,bool& aFlushTokens);
  nsresult ConsumeEndTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeAttributes(PRUnichar aChar, CToken* aToken, nsScanner& aScanner);
  nsresult ConsumeEntity(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeWhitespace(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeComment(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeNewline(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeText(CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeSpecialMarkup(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeProcessingInstruction(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);

  nsresult ScanDocStructure(bool aIsFinalChunk);

  static void AddToken(CToken*& aToken,nsresult aResult,nsDeque* aDeque,nsTokenAllocator* aTokenAllocator);

  nsDeque            mTokenDeque;
  bool               mIsFinalChunk;
  nsTokenAllocator*  mTokenAllocator;
  
  
  
  int32_t            mTokenScanPos;
  uint32_t           mFlags;
};

#endif


