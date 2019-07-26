






#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"
#include "nsCOMPtr.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"


#define CSS_REPORT_PARSE_ERRORS


#include "nsXPIDLString.h"
class nsIURI;


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

  void AppendToString(nsString& aBuffer);
};





class nsCSSScanner {
  public:
  nsCSSScanner();
  ~nsCSSScanner();

  
  
  
  void Init(const nsAString& aBuffer,
            nsIURI* aURI, uint32_t aLineNumber,
            nsCSSStyleSheet* aSheet, mozilla::css::Loader* aLoader);
  void Close();

  static bool InitGlobals();
  static void ReleaseGlobals();

  
  void SetSVGMode(bool aSVGMode) {
    mSVGMode = aSVGMode;
  }
  bool IsSVGMode() const {
    return mSVGMode;
  }

#ifdef CSS_REPORT_PARSE_ERRORS
  void AddToError(const nsSubstring& aErrorText);
  void OutputError();
  void ClearError();

  
  void ReportUnexpected(const char* aMessage);
  
private:
  void ReportUnexpectedParams(const char* aMessage,
                              const PRUnichar** aParams,
                              uint32_t aParamsLength);

public:
  template<uint32_t N>                           
  void ReportUnexpectedParams(const char* aMessage,
                              const PRUnichar* (&aParams)[N])
    {
      return ReportUnexpectedParams(aMessage, aParams, N);
    }
  
  void ReportUnexpectedEOF(const char* aLookingFor);
  
  void ReportUnexpectedEOF(PRUnichar aLookingFor);
  
  
  void ReportUnexpectedToken(nsCSSToken& tok, const char *aMessage);
  
  void ReportUnexpectedTokenParams(nsCSSToken& tok,
                                   const char* aMessage,
                                   const PRUnichar **aParams,
                                   uint32_t aParamsLength);
#endif

  uint32_t GetLineNumber() { return mLineNumber; }

  
  
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
  bool ParseAtKeyword(int32_t aChar, nsCSSToken& aResult);
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
  
  bool mSVGMode;
  bool mRecording;
  uint32_t mRecordStartOffset;

#ifdef CSS_REPORT_PARSE_ERRORS
  nsXPIDLCString mFileName;
  nsCOMPtr<nsIURI> mURI;  
  uint32_t mErrorLineNumber, mColNumber, mErrorColNumber;
  nsFixedString mError;
  PRUnichar mErrorBuf[200];
  uint64_t mInnerWindowID;
  bool mWindowIDCached;
  nsCSSStyleSheet* mSheet;
  mozilla::css::Loader* mLoader;
#endif
};

#endif 
