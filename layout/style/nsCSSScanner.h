








































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

  
  
  eCSSToken_Error           
};

struct nsCSSToken {
  nsCSSTokenType  mType;
  PRPackedBool    mIntegerValid; 
  PRPackedBool    mHasSign; 
  nsAutoString    mIdent NS_OKONHEAP;
  float           mNumber;
  PRInt32         mInteger;
  PRUnichar       mSymbol;

  nsCSSToken();

  PRBool IsDimension() {
    return PRBool((eCSSToken_Dimension == mType) ||
                  ((eCSSToken_Number == mType) && (mNumber == 0.0f)));
  }

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

  
  
  PRBool Next(nsresult& aErrorCode, nsCSSToken& aTokenResult);

  
  PRBool NextURL(nsresult& aErrorCode, nsCSSToken& aTokenResult);

  
  
  
  
  void Pushback(PRUnichar aChar);

  static inline PRBool
  IsIdentStart(PRInt32 aChar)
  {
    return aChar >= 0 &&
      (aChar >= 256 || (gLexTable[aChar] & START_IDENT) != 0);
  }

  static inline PRBool
  StartsIdent(PRInt32 aFirstChar, PRInt32 aSecondChar)
  {
    return IsIdentStart(aFirstChar) ||
      (aFirstChar == '-' && IsIdentStart(aSecondChar));
  }

  static PRBool IsWhitespace(PRInt32 ch) {
    return PRUint32(ch) < 256 && (gLexTable[ch] & IS_WHITESPACE) != 0;
  }

  static PRBool IsDigit(PRInt32 ch) {
    return PRUint32(ch) < 256 && (gLexTable[ch] & IS_DIGIT) != 0;
  }

  static PRBool IsHexDigit(PRInt32 ch) {
    return PRUint32(ch) < 256 && (gLexTable[ch] & IS_HEX_DIGIT) != 0;
  }

  static PRBool IsIdent(PRInt32 ch) {
    return ch >= 0 && (ch >= 256 || (gLexTable[ch] & IS_IDENT) != 0);
  }
  
protected:
  PRBool EnsureData(nsresult& aErrorCode);
  PRInt32 Read(nsresult& aErrorCode);
  PRInt32 Peek(nsresult& aErrorCode);
  PRBool LookAhead(nsresult& aErrorCode, PRUnichar aChar);
  PRBool EatWhiteSpace(nsresult& aErrorCode);
  PRBool EatNewline(nsresult& aErrorCode);

  void ParseAndAppendEscape(nsresult& aErrorCode, nsString& aOutput);
  PRBool ParseIdent(nsresult& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseAtKeyword(nsresult& aErrorCode, PRInt32 aChar,
                        nsCSSToken& aResult);
  PRBool ParseNumber(nsresult& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseRef(nsresult& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
  PRBool ParseString(nsresult& aErrorCode, PRInt32 aChar, nsCSSToken& aResult);
#if 0
  PRBool ParseEOLComment(nsresult& aErrorCode, nsCSSToken& aResult);
  PRBool ParseCComment(nsresult& aErrorCode, nsCSSToken& aResult);
#endif
  PRBool SkipCComment(nsresult& aErrorCode);

  PRBool GatherIdent(nsresult& aErrorCode, PRInt32 aChar, nsString& aIdent);

  
  nsCOMPtr<nsIUnicharInputStream> mInputStream;
  PRUnichar mBuffer[CSS_BUFFER_SIZE];

  const PRUnichar *mReadPointer;
  PRUint32 mOffset;
  PRUint32 mCount;
  PRUnichar* mPushback;
  PRInt32 mPushbackCount;
  PRInt32 mPushbackSize;
  PRUnichar mLocalPushback[4];

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

  static const PRUint8 IS_DIGIT;
  static const PRUint8 IS_HEX_DIGIT;
  static const PRUint8 START_IDENT;
  static const PRUint8 IS_IDENT;
  static const PRUint8 IS_WHITESPACE;

  static PRUint8 gLexTable[256];
  static void BuildLexTable();
};

#endif 
