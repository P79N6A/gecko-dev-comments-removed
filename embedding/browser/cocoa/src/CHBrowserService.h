




































#ifndef __CHBrowserService_h__
#define __CHBrowserService_h__

#import "nsAlertController.h"
#include "nsCOMPtr.h"
#include "nsIWindowCreator.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIFactory.h"




extern NSString* TermEmbeddingNotificationName;   
extern NSString* XPCOMShutDownNotificationName;   

class nsModuleComponentInfo;

class CHBrowserService :  public nsIWindowCreator,
                          public nsIFactory, 
                          public nsIHelperAppLauncherDialog
{
public:
  CHBrowserService();
  virtual ~CHBrowserService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIFACTORY
  NS_DECL_NSIHELPERAPPLAUNCHERDIALOG
  
  static nsresult InitEmbedding();
  static void TermEmbedding();
  static void BrowserClosed();
  
  
  
  
  
  static void RegisterAppComponents(const nsModuleComponentInfo* inComponents, const int inNumComponents);
  
  static void SetAlertController(nsAlertController* aController);
  static nsAlertController* GetAlertController();

public:
  static PRUint32 sNumBrowsers;

private:
  static void ShutDown();

  static CHBrowserService* sSingleton;
  static nsAlertController* sController;
  static PRBool sCanTerminate;
};


#endif 

