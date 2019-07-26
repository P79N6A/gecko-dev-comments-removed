






#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"

namespace mozilla {
namespace css {
class ErrorReporter;
}
}







enum nsCSSTokenType {
  
  
  
  eCSSToken_Whitespace,     

  
  
  
  
  
  eCSSToken_Ident,          
  eCSSToken_Function,       
  eCSSToken_AtKeyword,      
  eCSSToken_ID,             
  eCSSToken_Hash,           

  
  
  
  
  
  
  
  
  
  
  eCSSToken_Number,         
  eCSSToken_Dimension,      
  eCSSToken_Percentage,     

  
  
  
  
  
  eCSSToken_String,         
  eCSSToken_Bad_String,     
  eCSSToken_URL,            
  eCSSToken_Bad_URL,        

  
  eCSSToken_Symbol,         

  
  
  
  
  eCSSToken_Includes,       
  eCSSToken_Dashmatch,      
  eCSSToken_Beginsmatch,    
  eCSSToken_Endsmatch,      
  eCSSToken_Containsmatch,  

  
  
  
  
  
  
  
  
  eCSSToken_URange,         

  
  
  
  
  
  
  eCSSToken_HTMLComment,    
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

  nsCSSToken()
    : mNumber(0), mInteger(0), mInteger2(0), mType(eCSSToken_Whitespace),
      mSymbol('\0'), mIntegerValid(false), mHasSign(false)
  {}

  bool IsSymbol(PRUnichar aSymbol) const {
    return mType == eCSSToken_Symbol && mSymbol == aSymbol;
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

  
  
  
  bool Next(nsCSSToken& aTokenResult, bool aSkipWS);

  
  
  
  
  
  bool NextURL(nsCSSToken& aTokenResult);

  
  
  
  
  
  
  
  void Backup(uint32_t n);

  
  void StartRecording();

  
  void StopRecording();

  
  
  void StopRecording(nsString& aBuffer);

protected:
  int32_t Peek(uint32_t n = 0);
  void Advance(uint32_t n = 1);
  void AdvanceLine();

  int32_t Read();
  void Pushback(PRUnichar aChar);
  bool LookAhead(PRUnichar aChar);
  bool LookAheadOrEOF(PRUnichar aChar); 

  void SkipWhitespace();
  void SkipComment();

  bool GatherEscape(nsString& aOutput, bool aInString);
  bool GatherIdent(int32_t aChar, nsString& aIdent);

  bool ScanIdent(int32_t aChar, nsCSSToken& aResult);
  bool ScanAtKeyword(nsCSSToken& aResult);
  bool ScanHash(int32_t aChar, nsCSSToken& aResult);
  bool ScanNumber(int32_t aChar, nsCSSToken& aResult);
  bool ScanString(int32_t aChar, nsCSSToken& aResult);
  bool ScanURange(int32_t aChar, nsCSSToken& aResult);

  const PRUnichar *mBuffer;
  uint32_t mOffset;
  uint32_t mCount;

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
