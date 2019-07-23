





































#ifndef nsBaseCommandController_h__
#define nsBaseCommandController_h__

#define NS_BASECOMMANDCONTROLLER_CID \
{ 0xbf88b48c, 0xfd8e, 0x40b4, { 0xba, 0x36, 0xc7, 0xc3, 0xad, 0x6d, 0x8a, 0xc9 } }
#define NS_BASECOMMANDCONTROLLER_CONTRACTID \
 "@mozilla.org/embedcomp/base-command-controller;1"


#include "nsIController.h"
#include "nsIControllerContext.h"
#include "nsIControllerCommandTable.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"



class nsBaseCommandController :  public nsIController,
                            public nsIControllerContext,
                            public nsIInterfaceRequestor,
                            public nsICommandController
{
public:

          nsBaseCommandController();
  virtual ~nsBaseCommandController();

  
  NS_DECL_ISUPPORTS
    
  
  NS_DECL_NSICONTROLLER

  
  NS_DECL_NSICOMMANDCONTROLLER

  
  NS_DECL_NSICONTROLLERCONTEXT

  
  NS_DECL_NSIINTERFACEREQUESTOR
  
private:

   nsWeakPtr mCommandContextWeakPtr;
   nsISupports* mCommandContextRawPtr;
   
   
   nsCOMPtr<nsIControllerCommandTable> mCommandTable;     
};

#endif 

