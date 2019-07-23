










































#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"

class nsIContent;
class nsIURI;

#ifdef MOZ_IPC
namespace mozilla {
  namespace dom {
    class TabParent;
  }
}
#endif

class nsFrameLoader : public nsIFrameLoader
{
protected:
  nsFrameLoader(nsIContent *aOwner) :
    mOwnerContent(aOwner),
    mDepthTooGreat(PR_FALSE),
    mIsTopLevelContent(PR_FALSE),
    mDestroyCalled(PR_FALSE),
    mNeedsAsyncDestroy(PR_FALSE),
    mInSwap(PR_FALSE)
#ifdef MOZ_IPC
    , mChildProcess(nsnull)
    , mTriedNewProcess(PR_FALSE)
#endif
  {}

public:
  ~nsFrameLoader() {
    mNeedsAsyncDestroy = PR_TRUE;
    nsFrameLoader::Destroy();
  }

  static nsFrameLoader* Create(nsIContent* aOwner);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void Finalize();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);
private:

  NS_HIDDEN_(nsresult) EnsureDocShell();
  NS_HIDDEN_(void) GetURL(nsString& aURL);
  nsresult CheckURILoad(nsIURI* aURI);

#ifdef MOZ_IPC
  
  PRBool TryNewProcess();
#endif

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  nsIContent *mOwnerContent; 
  PRPackedBool mDepthTooGreat : 1;
  PRPackedBool mIsTopLevelContent : 1;
  PRPackedBool mDestroyCalled : 1;
  PRPackedBool mNeedsAsyncDestroy : 1;
  PRPackedBool mInSwap : 1;

#ifdef MOZ_IPC
  
  mozilla::dom::TabParent* mChildProcess;
  PRBool mTriedNewProcess;
#endif
};

#endif
