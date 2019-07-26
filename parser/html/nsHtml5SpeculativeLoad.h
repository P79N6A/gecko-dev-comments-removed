


 
#ifndef nsHtml5SpeculativeLoad_h
#define nsHtml5SpeculativeLoad_h

#include "nsString.h"

class nsHtml5TreeOpExecutor;

enum eHtml5SpeculativeLoad {
#ifdef DEBUG
  eSpeculativeLoadUninitialized,
#endif
  eSpeculativeLoadBase,
  eSpeculativeLoadImage,
  eSpeculativeLoadScript,
  eSpeculativeLoadScriptFromHead,
  eSpeculativeLoadStyle,
  eSpeculativeLoadManifest,
  eSpeculativeLoadSetDocumentCharset
};

class nsHtml5SpeculativeLoad {
  public:
    nsHtml5SpeculativeLoad();
    ~nsHtml5SpeculativeLoad();
    
    inline void InitBase(const nsAString& aUrl) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadBase;
      mUrl.Assign(aUrl);
    }

    inline void InitImage(const nsAString& aUrl,
                          const nsAString& aCrossOrigin) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadImage;
      mUrl.Assign(aUrl);
      mCrossOrigin.Assign(aCrossOrigin);
    }

    inline void InitScript(const nsAString& aUrl,
                           const nsAString& aCharset,
                           const nsAString& aType,
                           const nsAString& aCrossOrigin,
                           bool aParserInHead) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = aParserInHead ?
          eSpeculativeLoadScriptFromHead : eSpeculativeLoadScript;
      mUrl.Assign(aUrl);
      mCharset.Assign(aCharset);
      mTypeOrCharsetSource.Assign(aType);
      mCrossOrigin.Assign(aCrossOrigin);
    }
    
    inline void InitStyle(const nsAString& aUrl, const nsAString& aCharset,
			  const nsAString& aCrossOrigin) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadStyle;
      mUrl.Assign(aUrl);
      mCharset.Assign(aCharset);
      mCrossOrigin.Assign(aCrossOrigin);
    }

    










    inline void InitManifest(const nsAString& aUrl) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadManifest;
      mUrl.Assign(aUrl);
    }

    









    inline void InitSetDocumentCharset(nsACString& aCharset,
                                       int32_t aCharsetSource) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadSetDocumentCharset;
      CopyUTF8toUTF16(aCharset, mCharset);
      mTypeOrCharsetSource.Assign((char16_t)aCharsetSource);
    }

    void Perform(nsHtml5TreeOpExecutor* aExecutor);
    
  private:
    eHtml5SpeculativeLoad mOpCode;
    nsString mUrl;
    





    nsString mCharset;
    





    nsString mTypeOrCharsetSource;
    




    nsString mCrossOrigin;
};

#endif 
