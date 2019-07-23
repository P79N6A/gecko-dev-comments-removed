







































#include "nsHtml5SpeculativeLoader.h"
#include "nsICSSLoader.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsICSSLoaderObserver.h"





class nsHtml5DummyCSSLoaderObserver : public nsICSSLoaderObserver {
public:
  NS_IMETHOD
  StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate, nsresult aStatus) {
      return NS_OK;
  }
  NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS1(nsHtml5DummyCSSLoaderObserver, nsICSSLoaderObserver)

nsHtml5SpeculativeLoader::nsHtml5SpeculativeLoader(nsIDocument* aDocument)
  : mDocument(aDocument)
{
  MOZ_COUNT_CTOR(nsHtml5SpeculativeLoader);
  mPreloadedURLs.Init(23); 
}

nsHtml5SpeculativeLoader::~nsHtml5SpeculativeLoader()
{
  MOZ_COUNT_DTOR(nsHtml5SpeculativeLoader);
}

NS_IMPL_THREADSAFE_ADDREF(nsHtml5SpeculativeLoader)

NS_IMPL_THREADSAFE_RELEASE(nsHtml5SpeculativeLoader)

already_AddRefed<nsIURI>
nsHtml5SpeculativeLoader::ConvertIfNotPreloadedYet(const nsAString& aURL)
{
  nsIURI* base = mDocument->GetBaseURI();
  const nsCString& charset = mDocument->GetDocumentCharacterSet();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL, charset.get(), base);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create a URI");
    return nsnull;
  }
  nsCAutoString spec;
  uri->GetSpec(spec);
  if (mPreloadedURLs.Contains(spec)) {
    return nsnull;
  }
  mPreloadedURLs.Put(spec);
  nsIURI* retURI = uri;
  NS_ADDREF(retURI);
  return retURI;
}

void
nsHtml5SpeculativeLoader::PreloadScript(const nsAString& aURL,
                                        const nsAString& aCharset,
                                        const nsAString& aType)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->ScriptLoader()->PreloadURI(uri, aCharset, aType);
}

void
nsHtml5SpeculativeLoader::PreloadStyle(const nsAString& aURL,
                                       const nsAString& aCharset)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  nsCOMPtr<nsICSSLoaderObserver> obs = new nsHtml5DummyCSSLoaderObserver();
  mDocument->CSSLoader()->LoadSheet(uri, mDocument->NodePrincipal(),
                                    NS_LossyConvertUTF16toASCII(aCharset),
                                    obs);
}

void
nsHtml5SpeculativeLoader::PreloadImage(const nsAString& aURL)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  mDocument->MaybePreLoadImage(uri);
}
