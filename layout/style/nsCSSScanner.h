








































#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"
#include "nsCOMPtr.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"

class nsIUnicharInputStream;


#define CSS_REPORT_PARSE_ERRORS

#define CSS_BUFFER_SIZE 256


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
  PRInt32         mInteger;
  PRInt32         mInteger2;
  nsCSSTokenType  mType;
  PRUnichar       mSymbol;
  PRPackedBool    mIntegerValid; 
  PRPackedBool    mHasSign; 

  nsCSSToken();

  PRBool IsSymbol(PRUnichar aSymbol) {
    return PRBool((eCSSToken_Symbol == mType) && (mSymbol == aSymbol));
  }

  void AppendToString(nsString& aBuffer);
};





class nsCSSScanner {
  public:
  nsCSSScanner();
  ~nsCSSScanner();

  
  
  
  
  void Init(nsIUnicharInputStream* aInput, 
            const PRUnichar *aBuffer, PRUint32 aCount,
            nsIURI* aURI, PRUint32 aLineNumber,
            nsCSSStyleSheet* aSheet, mozilla::css::Loader* aLoader);
  void Close();

  static PRBool InitGlobals();
  static void ReleaseGlobals();

  
  void SetSVGMode(PRBool aSVGMode) {
    NS_ASSERTION(aSVGMode == PR_TRUE || aSVGMode == PR_FALSE,
                 "bad PRBool value");
    mSVGMode = aSVGMode;
  }
  PRBool IsSVGMode() const {
    return mSVGMode;
  }

#ifdef CSS_REPORT_PARSE_ERRORS
  void AddToError(const nsSubstring& aErrorText);
  void OutputError();
  void ClearError();

  
  void ReportUnexpected(const char* aMessage);
  void ReportUnexpectedParams(const char* aMessage,
                              const PRUnichar **aParams,
                              PRUint32 aParamsLength);
  
  void ReportUnexpectedEOF(const char* aLookingFor);
  
  void ReportUnexpectedEOF(PRUnichar aLookingFor);
  
  
  void ReportUnexpectedToken(nsCSSToken& tok, const char *aMessage);
  
  void ReportUnexpectedTokenParams(nsCSSToken& tok,
                                   const char* aMessage,
                                   const PRUnichar **aParams,
                                   PRUint32 aParamsLength);
#endif

  PRUint32 GetLineNumber() { return mLineNumber; }

  
  
  PRBool Next(nsCSSToken& aTokenResult);

  
  PRBool NextURL(nsCSSToken& aTokenResult);

  
  
  
  
  void Pushback(PRUnichar aChar);

  
  
  nsresult GetLowLevelError();

  
  void SetLowLevelError(nsresult aErrorCode);
  
protected:
  PRBool EnsureData();
  PRInt32 Read();
  PRInt32 Peek();
  PRBool LookAhead(PRUnichar aChar);
  PRBool LookAheadOrEOF(PRUnichar aChar); 
  void EatWhiteSpace();

  PRBool ParseAndAppendEscape(nsString& aOutput, PRBool aInString);
  PRBool ParseIdent(PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseAtKeyword(PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseNumber(PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseRef(PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseString(PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseURange(PRInt32 aChar, nsCSSToken& aResult);
  PRBool SkipCComment();

  PRBool GatherIdent(PRInt32 aChar, nsString& aIdent);

  
  nsCOMPtr<nsIUnicharInputStream> mInputStream;
  PRUnichar mBuffer[CSS_BUFFER_SIZE];

  const PRUnichar *mReadPointer;
  PRUint32 mOffset;
  PRUint32 mCount;
  PRUnichar* mPushback;
  PRInt32 mPushbackCount;
  PRInt32 mPushbackSize;
  PRUnichar mLocalPushback[4];
  nsresult mLowLevelError;

  PRUint32 mLineNumber;
  
  PRPackedBool mSVGMode;
#ifdef CSS_REPORT_PARSE_ERRORS
  nsXPIDLCString mFileName;
  nsCOMPtr<nsIURI> mURI;  
  PRUint32 mErrorLineNumber, mColNumber, mErrorColNumber;
  nsFixedString mError;
  PRUnichar mErrorBuf[200];
  PRUint64 mWindowID;
  PRBool mWindowIDCached;
  nsCSSStyleSheet* mSheet;
  mozilla::css::Loader* mLoader;
#endif
};

#endif 
