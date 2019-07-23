




































#ifndef nsHtml5SpeculativeLoader_h__
#define nsHtml5SpeculativeLoader_h__

#include "mozilla/Mutex.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsHashSets.h"

class nsHtml5SpeculativeLoader
{
  public:
    nsHtml5SpeculativeLoader(nsIDocument* aDocument);
    ~nsHtml5SpeculativeLoader();

    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);

    void PreloadScript(const nsAString& aURL,
                       const nsAString& aCharset,
                       const nsAString& aType);

    void PreloadStyle(const nsAString& aURL, const nsAString& aCharset);

    void PreloadImage(const nsAString& aURL);

  private:
    
    


    already_AddRefed<nsIURI> ConvertIfNotPreloadedYet(const nsAString& aURL);
  
    nsAutoRefCnt   mRefCnt;
    
    


    nsCOMPtr<nsIDocument> mDocument;
    
    


    nsCStringHashSet mPreloadedURLs;
};

#endif 