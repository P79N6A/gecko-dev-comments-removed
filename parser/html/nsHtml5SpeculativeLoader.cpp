







































#include "nsHtml5SpeculativeLoader.h"
#include "nsNetUtil.h"
#include "nsScriptLoader.h"
#include "nsIDocument.h"

nsHtml5SpeculativeLoader::
nsHtml5SpeculativeLoader(nsHtml5TreeOpExecutor* aExecutor)
  : mExecutor(aExecutor)
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
  nsIDocument* doc = mExecutor->GetDocument();
  if (!doc) {
    return nsnull;
  }
  nsIURI* base = doc->GetBaseURI();
  const nsCString& charset = doc->GetDocumentCharacterSet();
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
  nsIDocument* doc = mExecutor->GetDocument();
  if (doc) {
    doc->ScriptLoader()->PreloadURI(uri, aCharset, aType);
  }
}

void
nsHtml5SpeculativeLoader::PreloadStyle(const nsAString& aURL,
                                       const nsAString& aCharset)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  nsIDocument* doc = mExecutor->GetDocument();
  if (doc) {
    doc->PreloadStyle(uri, aCharset);
  }
}

void
nsHtml5SpeculativeLoader::PreloadImage(const nsAString& aURL)
{
  nsCOMPtr<nsIURI> uri = ConvertIfNotPreloadedYet(aURL);
  if (!uri) {
    return;
  }
  nsIDocument* doc = mExecutor->GetDocument();
  if (doc) {
    doc->MaybePreLoadImage(uri);
  }
}

void
nsHtml5SpeculativeLoader::ProcessManifest(const nsAString& aURL)
{
  mExecutor->ProcessOfflineManifest(aURL);
}
