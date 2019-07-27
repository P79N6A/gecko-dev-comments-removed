



#ifndef mozilla_embedding_PrintProgressDialogChild_h
#define mozilla_embedding_PrintProgressDialogChild_h

#include "mozilla/embedding/PPrintProgressDialogChild.h"
#include "nsIPrintProgressParams.h"
#include "nsIWebProgressListener.h"

class nsIObserver;

namespace mozilla {
namespace embedding {

class PrintProgressDialogChild final : public PPrintProgressDialogChild,
                                           public nsIWebProgressListener,
                                           public nsIPrintProgressParams
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIPRINTPROGRESSPARAMS

public:
  MOZ_IMPLICIT PrintProgressDialogChild(nsIObserver* aOpenObserver);

  virtual bool RecvDialogOpened() override;

private:
  virtual ~PrintProgressDialogChild();
  nsCOMPtr<nsIObserver> mOpenObserver;
  nsString mDocTitle;
  nsString mDocURL;
};

} 
} 

#endif
