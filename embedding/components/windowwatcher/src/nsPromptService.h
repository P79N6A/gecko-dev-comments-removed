




































#ifndef __nsPromptService_h
#define __nsPromptService_h


#define NS_PROMPTSERVICE_CID \
 {0xa2112d6a, 0x0e28, 0x421f, {0xb4, 0x6a, 0x25, 0xc0, 0xb3, 0x8, 0xcb, 0xd0}}

#define NS_NONBLOCKINGALERTSERVICE_CID \
 {0x150e7415, 0x72d7, 0x11da, {0xa9, 0x24, 0x00, 0x03, 0x93, 0x86, 0x35, 0x7a}}

#include "nsCOMPtr.h"
#include "nsIPromptService2.h"
#include "nsPIPromptService.h"
#include "nsINonBlockingAlertService.h"
#include "nsIWindowWatcher.h"

class nsIDOMWindow;
class nsIDialogParamBlock;

class nsPromptService: public nsIPromptService2,
                       public nsPIPromptService,
                       public nsINonBlockingAlertService {

public:

  nsPromptService();
  virtual ~nsPromptService();

  nsresult Init();

  NS_DECL_NSIPROMPTSERVICE
  NS_DECL_NSIPROMPTSERVICE2
  NS_DECL_NSPIPROMPTSERVICE
  NS_DECL_NSINONBLOCKINGALERTSERVICE
  NS_DECL_ISUPPORTS

private:
  nsresult GetLocaleString(const char *aKey, PRUnichar **aResult);

  nsCOMPtr<nsIWindowWatcher> mWatcher;
};

#endif

