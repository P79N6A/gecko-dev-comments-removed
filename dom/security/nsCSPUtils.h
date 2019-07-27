




#ifndef nsCSPUtils_h___
#define nsCSPUtils_h___

#include "nsCOMPtr.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"
#include "prlog.h"



void CSP_LogLocalizedStr(const char16_t* aName,
                         const char16_t** aParams,
                         uint32_t aLength,
                         const nsAString& aSourceName,
                         const nsAString& aSourceLine,
                         uint32_t aLineNumber,
                         uint32_t aColumnNumber,
                         uint32_t aFlags,
                         const char* aCategory,
                         uint32_t aInnerWindowID);

void CSP_GetLocalizedStr(const char16_t* aName,
                         const char16_t** aParams,
                         uint32_t aLength,
                         char16_t** outResult);

void CSP_LogStrMessage(const nsAString& aMsg);

void CSP_LogMessage(const nsAString& aMessage,
                    const nsAString& aSourceName,
                    const nsAString& aSourceLine,
                    uint32_t aLineNumber,
                    uint32_t aColumnNumber,
                    uint32_t aFlags,
                    const char* aCategory,
                    uint32_t aInnerWindowID);




#define INLINE_STYLE_VIOLATION_OBSERVER_TOPIC   "violated base restriction: Inline Stylesheets will not apply"
#define INLINE_SCRIPT_VIOLATION_OBSERVER_TOPIC  "violated base restriction: Inline Scripts will not execute"
#define EVAL_VIOLATION_OBSERVER_TOPIC           "violated base restriction: Code will not be created from strings"
#define SCRIPT_NONCE_VIOLATION_OBSERVER_TOPIC   "Inline Script had invalid nonce"
#define STYLE_NONCE_VIOLATION_OBSERVER_TOPIC    "Inline Style had invalid nonce"
#define SCRIPT_HASH_VIOLATION_OBSERVER_TOPIC    "Inline Script had invalid hash"
#define STYLE_HASH_VIOLATION_OBSERVER_TOPIC     "Inline Style had invalid hash"




static const char* CSPStrDirectives[] = {
  "-error-",    
  "default-src",     
  "script-src",      
  "object-src",      
  "style-src",       
  "img-src",         
  "media-src",       
  "frame-src",       
  "font-src",        
  "connect-src",     
  "report-uri",      
  "frame-ancestors", 
  "reflected-xss",   
  "base-uri",        
  "form-action",     
  "referrer"         
};

inline const char* CSP_CSPDirectiveToString(CSPDirective aDir)
{
  return CSPStrDirectives[static_cast<uint32_t>(aDir)];
}

inline CSPDirective CSP_StringToCSPDirective(const nsAString& aDir)
{
  nsString lowerDir = PromiseFlatString(aDir);
  ToLowerCase(lowerDir);

  uint32_t numDirs = (sizeof(CSPStrDirectives) / sizeof(CSPStrDirectives[0]));
  for (uint32_t i = 1; i < numDirs; i++) {
    if (lowerDir.EqualsASCII(CSPStrDirectives[i])) {
      return static_cast<CSPDirective>(i);
    }
  }
  NS_ASSERTION(false, "Can not convert unknown Directive to Integer");
  return nsIContentSecurityPolicy::NO_DIRECTIVE;
}




enum CSPKeyword {
  CSP_SELF = 0,
  CSP_UNSAFE_INLINE,
  CSP_UNSAFE_EVAL,
  CSP_NONE,
  CSP_NONCE,
  
  
  CSP_LAST_KEYWORD_VALUE,
  
  
  
  CSP_HASH
 };

static const char* CSPStrKeywords[] = {
  "'self'",          
  "'unsafe-inline'", 
  "'unsafe-eval'",   
  "'none'",          
  "'nonce-",         
  
};

inline const char* CSP_EnumToKeyword(enum CSPKeyword aKey)
{
  
  static_assert((sizeof(CSPStrKeywords) / sizeof(CSPStrKeywords[0]) ==
                static_cast<uint32_t>(CSP_LAST_KEYWORD_VALUE)),
                "CSP_LAST_KEYWORD_VALUE does not match length of CSPStrKeywords");
  return CSPStrKeywords[static_cast<uint32_t>(aKey)];
}

inline CSPKeyword CSP_KeywordToEnum(const nsAString& aKey)
{
  nsString lowerKey = PromiseFlatString(aKey);
  ToLowerCase(lowerKey);

  static_assert(CSP_LAST_KEYWORD_VALUE ==
                (sizeof(CSPStrKeywords) / sizeof(CSPStrKeywords[0])),
                 "CSP_LAST_KEYWORD_VALUE does not match length of CSPStrKeywords");

  for (uint32_t i = 0; i < CSP_LAST_KEYWORD_VALUE; i++) {
    if (lowerKey.EqualsASCII(CSPStrKeywords[i])) {
      return static_cast<CSPKeyword>(i);
    }
  }
  NS_ASSERTION(false, "Can not convert unknown Keyword to Enum");
  return CSP_LAST_KEYWORD_VALUE;
}



class nsCSPHostSrc;

nsCSPHostSrc* CSP_CreateHostSrcFromURI(nsIURI* aURI);
bool CSP_IsValidDirective(const nsAString& aDir);
bool CSP_IsDirective(const nsAString& aValue, CSPDirective aDir);
bool CSP_IsKeyword(const nsAString& aValue, enum CSPKeyword aKey);
bool CSP_IsQuotelessKeyword(const nsAString& aKey);
CSPDirective CSP_ContentTypeToDirective(nsContentPolicyType aType);




class nsCSPBaseSrc {
  public:
    nsCSPBaseSrc();
    virtual ~nsCSPBaseSrc();

    virtual bool permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const;
    virtual bool allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const;
    virtual void toString(nsAString& outStr) const = 0;
};



class nsCSPSchemeSrc : public nsCSPBaseSrc {
  public:
    explicit nsCSPSchemeSrc(const nsAString& aScheme);
    virtual ~nsCSPSchemeSrc();

    bool permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const;
    void toString(nsAString& outStr) const;

  private:
    nsString mScheme;
};



class nsCSPHostSrc : public nsCSPBaseSrc {
  public:
    explicit nsCSPHostSrc(const nsAString& aHost);
    virtual ~nsCSPHostSrc();

    bool permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const;
    void toString(nsAString& outStr) const;

    void setScheme(const nsAString& aScheme, bool aAllowHttps = false);
    void setPort(const nsAString& aPort);
    void appendPath(const nsAString &aPath);

  private:
    nsString mScheme;
    nsString mHost;
    nsString mPort;
    nsString mPath;
    bool     mAllowHttps;
};



class nsCSPKeywordSrc : public nsCSPBaseSrc {
  public:
    explicit nsCSPKeywordSrc(CSPKeyword aKeyword);
    virtual ~nsCSPKeywordSrc();

    bool allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const;
    void toString(nsAString& outStr) const;
    void invalidate();

  private:
    CSPKeyword mKeyword;
    
    bool       mInvalidated;
};



class nsCSPNonceSrc : public nsCSPBaseSrc {
  public:
    explicit nsCSPNonceSrc(const nsAString& aNonce);
    virtual ~nsCSPNonceSrc();

    bool permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const;
    bool allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const;
    void toString(nsAString& outStr) const;

  private:
    nsString mNonce;
};



class nsCSPHashSrc : public nsCSPBaseSrc {
  public:
    nsCSPHashSrc(const nsAString& algo, const nsAString& hash);
    virtual ~nsCSPHashSrc();

    bool allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const;
    void toString(nsAString& outStr) const;

  private:
    nsString mAlgorithm;
    nsString mHash;
};



class nsCSPReportURI : public nsCSPBaseSrc {
  public:
    explicit nsCSPReportURI(nsIURI* aURI);
    virtual ~nsCSPReportURI();

    void toString(nsAString& outStr) const;

  private:
    nsCOMPtr<nsIURI> mReportURI;
};



class nsCSPDirective {
  public:
    nsCSPDirective();
    explicit nsCSPDirective(CSPDirective aDirective);
    virtual ~nsCSPDirective();

    bool permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const;
    bool permits(nsIURI* aUri) const;
    bool allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const;
    void toString(nsAString& outStr) const;

    inline void addSrcs(const nsTArray<nsCSPBaseSrc*>& aSrcs)
      { mSrcs = aSrcs; }

    bool restrictsContentType(nsContentPolicyType aContentType) const;

    inline bool isDefaultDirective() const
     { return mDirective == nsIContentSecurityPolicy::DEFAULT_SRC_DIRECTIVE; }

    inline bool equals(CSPDirective aDirective) const
      { return (mDirective == aDirective); }

    void getReportURIs(nsTArray<nsString> &outReportURIs) const;

  private:
    CSPDirective            mDirective;
    nsTArray<nsCSPBaseSrc*> mSrcs;
};



class nsCSPPolicy {
  public:
    nsCSPPolicy();
    virtual ~nsCSPPolicy();

    bool permits(CSPDirective aDirective,
                 nsIURI* aUri,
                 const nsAString& aNonce,
                 bool aWasRedirected,
                 bool aSpecific,
                 nsAString& outViolatedDirective) const;
    bool permits(CSPDirective aDir,
                 nsIURI* aUri,
                 bool aSpecific) const;
    bool allows(nsContentPolicyType aContentType,
                enum CSPKeyword aKeyword,
                const nsAString& aHashOrNonce) const;
    bool allows(nsContentPolicyType aContentType,
                enum CSPKeyword aKeyword) const;
    void toString(nsAString& outStr) const;

    inline void addDirective(nsCSPDirective* aDir)
      { mDirectives.AppendElement(aDir); }

    bool hasDirective(CSPDirective aDir) const;

    inline void setReportOnlyFlag(bool aFlag)
      { mReportOnly = aFlag; }

    inline bool getReportOnlyFlag() const
      { return mReportOnly; }

    inline void setReferrerPolicy(const nsAString* aValue)
      { mReferrerPolicy = *aValue; }

    inline void getReferrerPolicy(nsAString& outPolicy) const
      { outPolicy.Assign(mReferrerPolicy); }

    void getReportURIs(nsTArray<nsString> &outReportURIs) const;

    void getDirectiveStringForContentType(nsContentPolicyType aContentType,
                                          nsAString& outDirective) const;

    void getDirectiveAsString(CSPDirective aDir, nsAString& outDirective) const;

    inline uint32_t getNumDirectives() const
      { return mDirectives.Length(); }

  private:
    nsTArray<nsCSPDirective*> mDirectives;
    bool                      mReportOnly;
    nsString                  mReferrerPolicy;
};

#endif 
