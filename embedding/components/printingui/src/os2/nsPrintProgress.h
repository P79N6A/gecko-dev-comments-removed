






































#ifndef __nsPrintProgress_h
#define __nsPrintProgress_h

#include "nsIPrintProgress.h"

#include "nsCOMPtr.h"
#include "nsISupportsArray.h"
#include "nsIDOMWindow.h"
#include "nsIPrintStatusFeedback.h"
#include "nsIObserver.h"
#include "nsString.h"

class nsPrintProgress : public nsIPrintProgress, public nsIPrintStatusFeedback
{
public: 
	NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTPROGRESS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIPRINTSTATUSFEEDBACK

	nsPrintProgress();
	virtual ~nsPrintProgress();

private:
  nsresult ReleaseListeners();

  bool                              m_closeProgress;
  bool                              m_processCanceled;
  nsString                          m_pendingStatus;
  PRInt32                           m_pendingStateFlags;
  PRInt32                           m_pendingStateValue;
  nsCOMPtr<nsIDOMWindow>            m_dialog;
  nsCOMPtr<nsISupportsArray>        m_listenerList;
  nsCOMPtr<nsIObserver>             m_observer;
};

#endif
