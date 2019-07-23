




































#ifndef __nsCocoaBrowserService_h__
#define __nsCocoaBrowserService_h__

#import "nsAlertController.h"
#include "nsEmbedAPI.h"
#include "nsIWindowCreator.h"
#include "nsIPromptService.h"

class nsCocoaBrowserService :  public nsIWindowCreator,
                               public nsIPromptService,
                               public nsIFactory
{
public:
  nsCocoaBrowserService();
  virtual ~nsCocoaBrowserService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIPROMPTSERVICE
  NS_DECL_NSIFACTORY

  static nsresult InitEmbedding();
  static void TermEmbedding();

  static void SetAlertController(nsAlertController* aController);
  nsAlertController* GetAlertController();

private:
  static PRUint32 sNumBrowsers;
  static nsCocoaBrowserService* sSingleton;
  static nsAlertController* sController;
};


#endif
