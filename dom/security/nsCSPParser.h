




#ifndef nsCSPParser_h___
#define nsCSPParser_h___

#include "nsCSPUtils.h"
#include "nsIURI.h"
#include "nsString.h"


























typedef nsTArray< nsTArray<nsString> > cspTokens;

class nsCSPTokenizer {

  public:
    static void tokenizeCSPPolicy(const nsAString &aPolicyString, cspTokens& outTokens);

  private:
    nsCSPTokenizer(const char16_t* aStart, const char16_t* aEnd);
    ~nsCSPTokenizer();

    inline bool atEnd()
    {
      return mCurChar >= mEndChar;
    }

    inline void skipWhiteSpace()
    {
      while (mCurChar < mEndChar && *mCurChar == ' ') {
        mCurToken.Append(*mCurChar++);
      }
      mCurToken.Truncate();
    }

    inline void skipWhiteSpaceAndSemicolon()
    {
      while (mCurChar < mEndChar && (*mCurChar == ' ' || *mCurChar == ';')) {
        mCurToken.Append(*mCurChar++);
      }
      mCurToken.Truncate();
    }

    inline bool accept(char16_t aChar)
    {
      NS_ASSERTION(mCurChar < mEndChar, "Trying to dereference mEndChar");
      if (*mCurChar == aChar) {
        mCurToken.Append(*mCurChar++);
        return true;
      }
      return false;
    }

    void generateNextToken();
    void generateTokens(cspTokens& outTokens);

    const char16_t* mCurChar;
    const char16_t* mEndChar;
    nsString        mCurToken;
};


class nsCSPParser {

  public:
    






    static nsCSPPolicy* parseContentSecurityPolicy(const nsAString &aPolicyString,
                                                   nsIURI *aSelfURI,
                                                   bool aReportOnly,
                                                   uint64_t aInnerWindowID);

  private:
    nsCSPParser(cspTokens& aTokens,
                nsIURI* aSelfURI,
                uint64_t aInnerWindowID);
    ~nsCSPParser();


    
    nsCSPPolicy*    policy();
    void            directive();
    nsCSPDirective* directiveName();
    void            directiveValue(nsTArray<nsCSPBaseSrc*>& outSrcs);
    void            referrerDirectiveValue();
    void            sourceList(nsTArray<nsCSPBaseSrc*>& outSrcs);
    nsCSPBaseSrc*   sourceExpression();
    nsCSPSchemeSrc* schemeSource();
    nsCSPHostSrc*   hostSource();
    nsCSPBaseSrc*   keywordSource();
    nsCSPNonceSrc*  nonceSource();
    nsCSPHashSrc*   hashSource();
    nsCSPHostSrc*   appHost(); 
    nsCSPHostSrc*   host();
    bool            hostChar();
    bool            schemeChar();
    bool            port();
    bool            path(nsCSPHostSrc* aCspHost);

    bool subHost();                                       
    bool atValidUnreservedChar();                         
    bool atValidSubDelimChar();                           
    bool atValidPctEncodedChar();                         
    bool subPath(nsCSPHostSrc* aCspHost);                 
    void reportURIList(nsTArray<nsCSPBaseSrc*>& outSrcs); 
    void percentDecodeStr(const nsAString& aEncStr,       
                          nsAString& outDecStr);

    inline bool atEnd()
    {
      return mCurChar >= mEndChar;
    }

    inline bool accept(char16_t aSymbol)
    {
      if (atEnd()) { return false; }
      return (*mCurChar == aSymbol) && advance();
    }

    inline bool accept(bool (*aClassifier) (char16_t))
    {
      if (atEnd()) { return false; }
      return (aClassifier(*mCurChar)) && advance();
    }

    inline bool peek(char16_t aSymbol)
    {
      if (atEnd()) { return false; }
      return *mCurChar == aSymbol;
    }

    inline bool peek(bool (*aClassifier) (char16_t))
    {
      if (atEnd()) { return false; }
      return aClassifier(*mCurChar);
    }

    inline bool advance()
    {
      if (atEnd()) { return false; }
      mCurValue.Append(*mCurChar++);
      return true;
    }

    inline void resetCurValue()
    {
      mCurValue.Truncate();
    }

    bool atEndOfPath();
    bool atValidPathChar();

    void resetCurChar(const nsAString& aToken);

    void logWarningErrorToConsole(uint32_t aSeverityFlag,
                                  const char* aProperty,
                                  const char16_t* aParams[],
                                  uint32_t aParamsLength);



































    const char16_t*    mCurChar;
    const char16_t*    mEndChar;
    nsString           mCurValue;
    nsString           mCurToken;
    nsTArray<nsString> mCurDir;

    
    bool               mHasHashOrNonce; 
    nsCSPKeywordSrc*   mUnsafeInlineKeywordSrc; 

    cspTokens          mTokens;
    nsIURI*            mSelfURI;
    nsCSPPolicy*       mPolicy;
    uint64_t           mInnerWindowID; 
};

#endif 
