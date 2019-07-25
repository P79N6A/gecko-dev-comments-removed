



































 
#ifndef nsHtml5SpeculativeLoad_h_
#define nsHtml5SpeculativeLoad_h_

#include "nsString.h"

class nsHtml5TreeOpExecutor;

enum eHtml5SpeculativeLoad {
#ifdef DEBUG
  eSpeculativeLoadUninitialized,
#endif
  eSpeculativeLoadBase,
  eSpeculativeLoadImage,
  eSpeculativeLoadScript,
  eSpeculativeLoadStyle,
  eSpeculativeLoadManifest  
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

    inline void InitImage(const nsAString& aUrl) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadImage;
      mUrl.Assign(aUrl);
    }

    inline void InitScript(const nsAString& aUrl,
                      const nsAString& aCharset,
                      const nsAString& aType) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadScript;
      mUrl.Assign(aUrl);
      mCharset.Assign(aCharset);
      mType.Assign(aType);
    }
    
    inline void InitStyle(const nsAString& aUrl, const nsAString& aCharset) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadStyle;
      mUrl.Assign(aUrl);
      mCharset.Assign(aCharset);
    }

    










    inline void InitManifest(const nsAString& aUrl) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadManifest;
      mUrl.Assign(aUrl);
    }

    void Perform(nsHtml5TreeOpExecutor* aExecutor);
    
  private:
    eHtml5SpeculativeLoad mOpCode;
    nsString mUrl;
    nsString mCharset;
    nsString mType;
};

#endif 
