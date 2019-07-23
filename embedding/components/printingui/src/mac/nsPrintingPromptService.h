




































#ifndef __nsPrintingPromptService_h
#define __nsPrintingPromptService_h


#define NS_PRINTINGPROMPTSERVICE_CID \
 {0xe042570c, 0x62de, 0x4bb6, { 0xa6, 0xe0, 0x79, 0x8e, 0x3c, 0x7, 0xb4, 0xdf}}
#define NS_PRINTINGPROMPTSERVICE_CONTRACTID \
 "@mozilla.org/embedcomp/printingprompt-service;1"

#include "nsCOMPtr.h"
#include "nsIPrintingPromptService.h"
#include "nsPIPromptService.h"
#include "nsIWindowWatcher.h"


#include "nsPrintProgress.h"
#include "nsPrintProgressParams.h"
#include "nsIWebProgressListener.h"


class nsIDOMWindow;
class nsIDialogParamBlock;

class nsPrintingPromptService: public nsIPrintingPromptService,
                               public nsIWebProgressListener
{
public:
  nsPrintingPromptService();
  virtual ~nsPrintingPromptService();

  nsresult Init();

  NS_DECL_NSIPRINTINGPROMPTSERVICE
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_ISUPPORTS

private:
  nsCOMPtr<nsIWindowWatcher> mWatcher;
  nsCOMPtr<nsIPrintProgress> mPrintProgress;
  nsCOMPtr<nsIWebProgressListener> mWebProgressListener;  
};

#endif
