










































#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"

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

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  void Finalize();
private:
  ~nsFrameLoader() {
    mInDestructor = PR_TRUE;
    nsFrameLoader::Destroy();
  }

  NS_HIDDEN_(nsresult) EnsureDocShell();
  NS_HIDDEN_(void) GetURL(nsString& aURL);

  nsCOMPtr<nsIDocShell> mDocShell;

  nsIContent *mOwnerContent; 
  PRPackedBool mDepthTooGreat;
  PRPackedBool mIsTopLevelContent;
  PRPackedBool mDestroyCalled;
  PRPackedBool mInDestructor;
};

#endif
