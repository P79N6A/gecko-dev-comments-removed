










































#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"
#include "nsSize.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"
#include "nsFrameMessageManager.h"

class nsIContent;
class nsIURI;
class nsIFrameFrame;
class nsIView;

#ifdef MOZ_IPC
namespace mozilla {
  namespace dom {
    class TabParent;
    class PIFrameEmbeddingParent;
  }
}

#ifdef MOZ_WIDGET_GTK2
typedef struct _GtkWidget GtkWidget;
#endif
#ifdef MOZ_WIDGET_QT
class QX11EmbedContainer;
#endif
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
    , mRemoteWidgetCreated(PR_FALSE)
    , mRemoteFrame(false)
    , mChildProcess(nsnull)
#if defined(MOZ_WIDGET_GTK2) || defined(MOZ_WIDGET_QT)
    , mRemoteSocket(nsnull)
#endif
#endif
  {}

public:
  ~nsFrameLoader() {
    mNeedsAsyncDestroy = PR_TRUE;
    if (mMessageManager) {
      mMessageManager->Disconnect();
    }
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

  nsresult CreateStaticClone(nsIFrameLoader* aDest);

  



  bool Show(PRInt32 marginWidth, PRInt32 marginHeight,
            PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
            nsIFrameFrame* frame);

  




  void Hide();

  nsresult CloneForStatic(nsIFrameLoader* aOriginal);

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);

#ifdef MOZ_IPC
  mozilla::dom::PIFrameEmbeddingParent* GetChildProcess();
#endif

  nsFrameMessageManager* GetFrameMessageManager() { return mMessageManager; }

private:

#ifdef MOZ_IPC
  bool ShouldUseRemoteProcess();
#endif

  



  nsresult MaybeCreateDocShell();
  void GetURL(nsString& aURL);

  
  NS_HIDDEN_(nsIntSize) GetSubDocumentSize(const nsIFrame *aIFrame);

  
  
  NS_HIDDEN_(nsresult) UpdateBaseWindowPositionAndSize(nsIFrame *aIFrame);
  nsresult CheckURILoad(nsIURI* aURI);
  void FireErrorEvent();
  nsresult ReallyStartLoadingInternal();

#ifdef MOZ_IPC
  
  bool TryNewProcess();

  
  
  bool ShowRemoteFrame(nsIFrameFrame* frame, nsIView* view);
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
  PRPackedBool mRemoteWidgetCreated : 1;
  bool mRemoteFrame;
  
  nsCOMPtr<nsIObserver> mChildHost;
  mozilla::dom::TabParent* mChildProcess;

#ifdef MOZ_WIDGET_GTK2
  GtkWidget* mRemoteSocket;
#elif defined(MOZ_WIDGET_QT)
  QX11EmbedContainer* mRemoteSocket;
#endif
#endif
  nsRefPtr<nsFrameMessageManager> mMessageManager;
};

#endif
