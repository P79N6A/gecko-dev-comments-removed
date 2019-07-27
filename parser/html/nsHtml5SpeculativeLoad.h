


 
#ifndef nsHtml5SpeculativeLoad_h
#define nsHtml5SpeculativeLoad_h

#include "nsString.h"
#include "nsContentUtils.h"

class nsHtml5TreeOpExecutor;

enum eHtml5SpeculativeLoad {
#ifdef DEBUG
  eSpeculativeLoadUninitialized,
#endif
  eSpeculativeLoadBase,
  eSpeculativeLoadMetaReferrer,
  eSpeculativeLoadImage,
  eSpeculativeLoadOpenPicture,
  eSpeculativeLoadEndPicture,
  eSpeculativeLoadPictureSource,
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

    inline void InitBase(const nsAString& aUrl)
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadBase;
      mUrl.Assign(aUrl);
    }

    inline void InitMetaReferrerPolicy(const nsAString& aReferrerPolicy) {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadMetaReferrer;
      mMetaReferrerPolicy.Assign(
        nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(aReferrerPolicy));
    }

    inline void InitImage(const nsAString& aUrl,
                          const nsAString& aCrossOrigin,
                          const nsAString& aSrcset,
                          const nsAString& aSizes)
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadImage;
      mUrl.Assign(aUrl);
      mCrossOrigin.Assign(aCrossOrigin);
      mSrcset.Assign(aSrcset);
      mSizes.Assign(aSizes);
    }

    
    
    
    
    
    
    
    inline void InitOpenPicture()
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadOpenPicture;
    }

    inline void InitEndPicture()
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadEndPicture;
    }

    inline void InitPictureSource(const nsAString& aSrcset,
                                  const nsAString& aSizes,
                                  const nsAString& aType,
                                  const nsAString& aMedia)
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadPictureSource;
      mSrcset.Assign(aSrcset);
      mSizes.Assign(aSizes);
      mTypeOrCharsetSource.Assign(aType);
      mMedia.Assign(aMedia);
    }

    inline void InitScript(const nsAString& aUrl,
                           const nsAString& aCharset,
                           const nsAString& aType,
                           const nsAString& aCrossOrigin,
                           bool aParserInHead)
    {
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
                          const nsAString& aCrossOrigin)
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadStyle;
      mUrl.Assign(aUrl);
      mCharset.Assign(aCharset);
      mCrossOrigin.Assign(aCrossOrigin);
    }

    










    inline void InitManifest(const nsAString& aUrl)
    {
      NS_PRECONDITION(mOpCode == eSpeculativeLoadUninitialized,
                      "Trying to reinitialize a speculative load!");
      mOpCode = eSpeculativeLoadManifest;
      mUrl.Assign(aUrl);
    }

    









    inline void InitSetDocumentCharset(nsACString& aCharset,
                                       int32_t aCharsetSource)
    {
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
    nsString mMetaReferrerPolicy;
    





    nsString mCharset;
    





    nsString mTypeOrCharsetSource;
    




    nsString mCrossOrigin;
    




    nsString mSrcset;
    



    nsString mSizes;
    



    nsString mMedia;
};

#endif 
