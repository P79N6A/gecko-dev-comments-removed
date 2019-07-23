








































#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"
#include "nsCOMPtr.h"
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
  eCSSToken_InvalidURL,     

  eCSSToken_HTMLComment,    

  eCSSToken_Includes,       
  eCSSToken_Dashmatch,      
  eCSSToken_Beginsmatch,    
  eCSSToken_Endsmatch,      
  eCSSToken_Containsmatch,  

  eCSSToken_URange,         
                            
                            
                            

  
  
  eCSSToken_Error           
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
            nsIURI* aURI, PRUint32 aLineNumber);
  void Close();

  static PRBool InitGlobals();
  static void ReleaseGlobals();

#ifdef  MOZ_SVG
  
  void SetSVGMode(PRBool aSVGMode) {
    NS_ASSERTION(aSVGMode == PR_TRUE || aSVGMode == PR_FALSE,
                 "bad PRBool value");
    mSVGMode = aSVGMode;
  }
  PRBool IsSVGMode() const {
    return mSVGMode;
  }

#endif
#ifdef CSS_REPORT_PARSE_ERRORS
  NS_HIDDEN_(void) AddToError(const nsSubstring& aErrorText);
  NS_HIDDEN_(void) OutputError();
  NS_HIDDEN_(void) ClearError();

  
  NS_HIDDEN_(void) ReportUnexpected(const char* aMessage);
  NS_HIDDEN_(void) ReportUnexpectedParams(const char* aMessage,
                                          const PRUnichar **aParams,
                                          PRUint32 aParamsLength);
  
  NS_HIDDEN_(void) ReportUnexpectedEOF(const char* aLookingFor);
  
  NS_HIDDEN_(void) ReportUnexpectedEOF(PRUnichar aLookingFor);
  
  
  NS_HIDDEN_(void) ReportUnexpectedToken(nsCSSToken& tok,
                                         const char *aMessage);
  
  NS_HIDDEN_(void) ReportUnexpectedTokenParams(nsCSSToken& tok,
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
  void EatWhiteSpace();
  
  void ParseAndAppendEscape(nsString& aOutput);
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
#ifdef MOZ_SVG
  
  PRPackedBool mSVGMode;
#endif
#ifdef CSS_REPORT_PARSE_ERRORS
  nsXPIDLCString mFileName;
  nsCOMPtr<nsIURI> mURI;  
  PRUint32 mErrorLineNumber, mColNumber, mErrorColNumber;
  nsFixedString mError;
  PRUnichar mErrorBuf[200];
#endif
};

#endif 
