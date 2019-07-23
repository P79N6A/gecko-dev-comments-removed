











































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
  nsHTMLTokenizer(PRInt32 aParseMode = eDTDMode_quirks,
                  eParserDocType aDocType = eHTML3_Quirks,
                  eParserCommands aCommand = eViewNormal,
                  PRUint16 aFlags = 0);
  virtual ~nsHTMLTokenizer();

protected:

  nsresult ConsumeTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner,PRBool& aFlushTokens);
  nsresult ConsumeStartTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner,PRBool& aFlushTokens);
  nsresult ConsumeEndTag(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeAttributes(PRUnichar aChar, CToken* aToken, nsScanner& aScanner);
  nsresult ConsumeEntity(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeWhitespace(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeComment(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeNewline(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeText(CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeSpecialMarkup(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);
  nsresult ConsumeProcessingInstruction(PRUnichar aChar,CToken*& aToken,nsScanner& aScanner);

  nsresult ScanDocStructure(PRBool aIsFinalChunk);

  static void AddToken(CToken*& aToken,nsresult aResult,nsDeque* aDeque,nsTokenAllocator* aTokenAllocator);

  nsDeque            mTokenDeque;
  PRPackedBool       mIsFinalChunk;
  nsTokenAllocator*  mTokenAllocator;
  
  
  
  PRInt32            mTokenScanPos;
  PRUint32           mFlags;
};

#endif


