






#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"

namespace mozilla {
namespace css {
class ErrorReporter;
}
}


enum nsCSSTokenType {
  
  eCSSToken_Ident,          

  
  eCSSToken_AtKeyword,      

  
  
  eCSSToken_Number,         
  eCSSToken_Percentage,     
  eCSSToken_Dimension,      

  
  eCSSToken_String,         

  
  eCSSToken_WhiteSpace,     

  
  eCSSToken_Symbol,         

  
  eCSSToken_ID,             
  
  
  
  eCSSToken_Ref,            

  eCSSToken_Function,       

  eCSSToken_URL,            
  eCSSToken_Bad_URL,        

  eCSSToken_HTMLComment,    

  eCSSToken_Includes,       
  eCSSToken_Dashmatch,      
  eCSSToken_Beginsmatch,    
  eCSSToken_Endsmatch,      
  eCSSToken_Containsmatch,  

  eCSSToken_URange,         
                            
                            
                            

  
  eCSSToken_Bad_String      
};

struct nsCSSToken {
  nsAutoString    mIdent NS_OKONHEAP;
  float           mNumber;
  int32_t         mInteger;
  int32_t         mInteger2;
  nsCSSTokenType  mType;
  PRUnichar       mSymbol;
  bool            mIntegerValid; 
  bool            mHasSign; 

  nsCSSToken();

  bool IsSymbol(PRUnichar aSymbol) {
    return bool((eCSSToken_Symbol == mType) && (mSymbol == aSymbol));
  }

  void AppendToString(nsString& aBuffer) const;
};





class nsCSSScanner {
  public:
  
  
  nsCSSScanner(const nsAString& aBuffer, uint32_t aLineNumber);
  ~nsCSSScanner();

  void SetErrorReporter(mozilla::css::ErrorReporter* aReporter) {
    mReporter = aReporter;
  }
  
  void SetSVGMode(bool aSVGMode) {
    mSVGMode = aSVGMode;
  }
  bool IsSVGMode() const {
    return mSVGMode;
  }

  
  
  uint32_t GetLineNumber() const { return mTokenLineNumber; }

  
  
  uint32_t GetColumnNumber() const
  { return mTokenOffset - mTokenLineOffset; }

  
  
  nsDependentSubstring GetCurrentLine() const;

  
  
  bool Next(nsCSSToken& aTokenResult);

  
  bool NextURL(nsCSSToken& aTokenResult);

  
  
  
  
  void Pushback(PRUnichar aChar);

  
  void StartRecording();

  
  void StopRecording();

  
  
  void StopRecording(nsString& aBuffer);

protected:
  int32_t Read();
  int32_t Peek();
  bool LookAhead(PRUnichar aChar);
  bool LookAheadOrEOF(PRUnichar aChar); 
  void EatWhiteSpace();

  bool ParseAndAppendEscape(nsString& aOutput, bool aInString);
  bool ParseIdent(int32_t aChar, nsCSSToken& aResult);
  bool ParseAtKeyword(nsCSSToken& aResult);
  bool ParseNumber(int32_t aChar, nsCSSToken& aResult);
  bool ParseRef(int32_t aChar, nsCSSToken& aResult);
  bool ParseString(int32_t aChar, nsCSSToken& aResult);
  bool ParseURange(int32_t aChar, nsCSSToken& aResult);
  bool SkipCComment();

  bool GatherIdent(int32_t aChar, nsString& aIdent);

  const PRUnichar *mReadPointer;
  uint32_t mOffset;
  uint32_t mCount;

  PRUnichar* mPushback;
  int32_t mPushbackCount;
  int32_t mPushbackSize;
  PRUnichar mLocalPushback[4];

  uint32_t mLineNumber;
  uint32_t mLineOffset;

  uint32_t mTokenLineNumber;
  uint32_t mTokenLineOffset;
  uint32_t mTokenOffset;

  uint32_t mRecordStartOffset;

  mozilla::css::ErrorReporter *mReporter;

  
  bool mSVGMode;
  bool mRecording;
};

#endif 
