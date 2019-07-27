



#include "mozilla/unused.h"
#include "nsIObserver.h"
#include "PrintProgressDialogChild.h"

class nsIWebProgress;
class nsIRequest;

using mozilla::unused;

namespace mozilla {
namespace embedding {

NS_IMPL_ISUPPORTS(PrintProgressDialogChild,
                  nsIWebProgressListener,
                  nsIPrintProgressParams)

PrintProgressDialogChild::PrintProgressDialogChild(
  nsIObserver* aOpenObserver) :
  mOpenObserver(aOpenObserver)
{
  MOZ_COUNT_CTOR(PrintProgressDialogChild);
}

PrintProgressDialogChild::~PrintProgressDialogChild()
{
  
  
  
  
  unused << Send__delete__(this);
  MOZ_COUNT_DTOR(PrintProgressDialogChild);
}

bool
PrintProgressDialogChild::RecvDialogOpened()
{
  
  
  
  mOpenObserver->Observe(nullptr, nullptr, nullptr);
  return true;
}



NS_IMETHODIMP
PrintProgressDialogChild::OnStateChange(nsIWebProgress* aProgress,
                                        nsIRequest* aRequest,
                                        uint32_t aStateFlags,
                                        nsresult aStatus)
{
  unused << SendStateChange(aStateFlags, aStatus);
  return NS_OK;
}

NS_IMETHODIMP
PrintProgressDialogChild::OnProgressChange(nsIWebProgress * aProgress,
                                           nsIRequest * aRequest,
                                           int32_t aCurSelfProgress,
                                           int32_t aMaxSelfProgress,
                                           int32_t aCurTotalProgress,
                                           int32_t aMaxTotalProgress)
{
  unused << SendProgressChange(aCurSelfProgress, aMaxSelfProgress,
                               aCurTotalProgress, aMaxTotalProgress);
  return NS_OK;
}

NS_IMETHODIMP
PrintProgressDialogChild::OnLocationChange(nsIWebProgress* aProgress,
                                           nsIRequest* aRequest,
                                           nsIURI* aURI,
                                           uint32_t aFlags)
{
  return NS_OK;
}

NS_IMETHODIMP
PrintProgressDialogChild::OnStatusChange(nsIWebProgress* aProgress,
                                         nsIRequest* aRequest,
                                         nsresult aStatus,
                                         const char16_t* aMessage)
{
  return NS_OK;
}

NS_IMETHODIMP
PrintProgressDialogChild::OnSecurityChange(nsIWebProgress* aProgress,
                                           nsIRequest* aRequest,
                                           uint32_t aState)
{
  return NS_OK;
}



NS_IMETHODIMP PrintProgressDialogChild::GetDocTitle(char16_t* *aDocTitle)
{
  NS_ENSURE_ARG(aDocTitle);

  *aDocTitle = ToNewUnicode(mDocTitle);
  return NS_OK;
}

NS_IMETHODIMP PrintProgressDialogChild::SetDocTitle(const char16_t* aDocTitle)
{
  mDocTitle = aDocTitle;
  unused << SendDocTitleChange(nsString(aDocTitle));
  return NS_OK;
}

NS_IMETHODIMP PrintProgressDialogChild::GetDocURL(char16_t **aDocURL)
{
  NS_ENSURE_ARG(aDocURL);

  *aDocURL = ToNewUnicode(mDocURL);
  return NS_OK;
}

NS_IMETHODIMP PrintProgressDialogChild::SetDocURL(const char16_t* aDocURL)
{
  mDocURL = aDocURL;
  unused << SendDocURLChange(nsString(aDocURL));
  return NS_OK;
}

} 
} 
