










































#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"
#include "nsSize.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"

class nsIContent;
class nsIURI;
class nsIFrameFrame;

#ifdef MOZ_IPC
namespace mozilla {
  namespace dom {
    class TabParent;
    class PIFrameEmbeddingParent;
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
    , mRemoteFrame(false)
    , mChildProcess(nsnull)
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

  



  bool Show(PRInt32 marginWidth, PRInt32 marginHeight,
            PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
            nsIFrameFrame* frame);

  




  void Hide();

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);

#ifdef MOZ_IPC
  mozilla::dom::PIFrameEmbeddingParent* GetChildProcess();
#endif

private:

#ifdef MOZ_IPC
  bool ShouldUseRemoteProcess();
#endif

  



  nsresult MaybeCreateDocShell();
  void GetURL(nsString& aURL);

  
  NS_HIDDEN_(nsIntSize) GetSubDocumentSize(const nsIFrame *aIFrame);

  
  
  NS_HIDDEN_(nsresult) UpdateBaseWindowPositionAndSize(nsIFrame *aIFrame);
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
  bool mRemoteFrame;
  
  mozilla::dom::TabParent* mChildProcess;
#endif
};

#endif
