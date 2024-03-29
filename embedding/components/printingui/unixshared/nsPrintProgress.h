




#ifndef __nsPrintProgress_h
#define __nsPrintProgress_h

#include "nsIPrintProgress.h"
#include "nsIPrintingPromptService.h"

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsIPrintStatusFeedback.h"
#include "nsIObserver.h"
#include "nsString.h"

class nsPrintProgress : public nsIPrintProgress, public nsIPrintStatusFeedback
{
public: 
	NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPRINTPROGRESS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIPRINTSTATUSFEEDBACK

  explicit nsPrintProgress(nsIPrintSettings* aPrintSettings);

protected:
	virtual ~nsPrintProgress();

private:
  nsresult ReleaseListeners();

  bool                              m_closeProgress;
  bool                              m_processCanceled;
  nsString                          m_pendingStatus;
  int32_t                           m_pendingStateFlags;
  nsresult                          m_pendingStateValue;
  nsCOMPtr<nsIDOMWindow>            m_dialog;
  nsCOMArray<nsIWebProgressListener>        m_listenerList;
  nsCOMPtr<nsIObserver>             m_observer;
  nsCOMPtr<nsIPrintSettings>        m_PrintSetting;
};

#endif
