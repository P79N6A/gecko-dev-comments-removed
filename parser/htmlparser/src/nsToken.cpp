




































#include "nsToken.h"
#include "nsScanner.h"

static int TokenCount=0;
static int DelTokenCount=0;

int CToken::GetTokenCount() {
  return TokenCount-DelTokenCount;
}











CToken::CToken(PRInt32 aTag) {
  
  
#ifdef MATCH_CTOR_DTOR 
  MOZ_COUNT_CTOR(CToken);
#endif 
  mAttrCount=0;
  mNewlineCount=0;
  mLineNumber = 0;
  mInError = PR_FALSE;
  mTypeID=aTag;
  
  
  
  
  
  mUseCount=1;

#ifdef NS_DEBUG
  ++TokenCount;
#endif
}






CToken::~CToken() {
  
  
#ifdef MATCH_CTOR_DTOR 
  MOZ_COUNT_DTOR(CToken);
#endif
  ++DelTokenCount;
  mUseCount=0;
}

 









nsresult CToken::Consume(PRUnichar aChar,nsScanner& aScanner,PRInt32 aMode) {
  nsresult result=NS_OK;
  return result;
}







void CToken::GetSource(nsString& anOutputString) {
  anOutputString.Assign(GetStringValue());
}





void CToken::AppendSourceTo(nsAString& anOutputString) {
  anOutputString.Append(GetStringValue());
}








PRInt32 CToken::GetTypeID(void) {
  return mTypeID;
}







PRInt16 CToken::GetAttributeCount(void) {
  return mAttrCount;
}









PRInt32 CToken::GetTokenType(void) {
  return -1;
}






void CToken::SelfTest(void) {
#ifdef _DEBUG
#endif
}


