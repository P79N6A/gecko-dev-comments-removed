










































#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"
#include "nsIURI.h"

class nsIContent;
class nsIURI;

class nsFrameLoader : public nsIFrameLoader
{
public:
  nsFrameLoader(nsIContent *aOwner) :
    mOwnerContent(aOwner),
    mDepthTooGreat(PR_FALSE),
    mIsTopLevelContent(PR_FALSE),
    mDestroyCalled(PR_FALSE),
    mInDestructor(PR_FALSE)
  {}

  ~nsFrameLoader() {
    mInDestructor = PR_TRUE;
    nsFrameLoader::Destroy();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void Finalize();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }
private:

  NS_HIDDEN_(nsresult) EnsureDocShell();
  NS_HIDDEN_(void) GetURL(nsString& aURL);
  nsresult CheckURILoad(nsIURI* aURI);

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  nsIContent *mOwnerContent; 
  PRPackedBool mDepthTooGreat;
  PRPackedBool mIsTopLevelContent;
  PRPackedBool mDestroyCalled;
  PRPackedBool mInDestructor;
};

#endif
