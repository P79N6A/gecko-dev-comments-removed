




#include "nsToken.h"
#include "nsScanner.h"

static int TokenCount=0;
static int DelTokenCount=0;

int CToken::GetTokenCount() {
  return TokenCount-DelTokenCount;
}











CToken::CToken(int32_t aTag) {
  mAttrCount=0;
  mNewlineCount=0;
  mLineNumber = 0;
  mInError = false;
  mTypeID=aTag;
  
  
  
  
  
  mUseCount=1;
  NS_LOG_ADDREF(this, 1, "CToken", sizeof(*this));

#ifdef DEBUG
  ++TokenCount;
#endif
}






CToken::~CToken() {
  ++DelTokenCount;
#ifdef NS_BUILD_REFCNT_LOGGING
  if (mUseCount == 1) {
    
    NS_LOG_RELEASE(this, 0, "CToken");
  }
#endif
  mUseCount=0;
}

 









nsresult CToken::Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode) {
  nsresult result=NS_OK;
  return result;
}







void CToken::GetSource(nsString& anOutputString) {
  anOutputString.Assign(GetStringValue());
}





void CToken::AppendSourceTo(nsAString& anOutputString) {
  anOutputString.Append(GetStringValue());
}








int32_t CToken::GetTypeID(void) {
  return mTypeID;
}







int16_t CToken::GetAttributeCount(void) {
  return mAttrCount;
}









int32_t CToken::GetTokenType(void) {
  return -1;
}






void CToken::SelfTest(void) {
#ifdef _DEBUG
#endif
}


