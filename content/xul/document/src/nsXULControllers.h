










#ifndef nsXULControllers_h__
#define nsXULControllers_h__

#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsIControllers.h"
#include "nsISecurityCheckedComponent.h"
#include "nsCycleCollectionParticipant.h"


class nsXULControllerData
{
public:
                            nsXULControllerData(PRUint32 inControllerID, nsIController* inController)
                            : mControllerID(inControllerID)
                            , mController(inController)
                            {                            
                            }

                            ~nsXULControllerData() {}

    PRUint32                GetControllerID()   { return mControllerID; }

    nsresult                GetController(nsIController **outController)
                            {
                              NS_IF_ADDREF(*outController = mController);
                              return NS_OK;
                            }
    
    PRUint32                mControllerID;
    nsCOMPtr<nsIController> mController;
};


nsresult NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);

class nsXULControllers : public nsIControllers,
                         public nsISecurityCheckedComponent
{
public:
    friend nsresult
    NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULControllers, nsIControllers)
    NS_DECL_NSICONTROLLERS
    NS_DECL_NSISECURITYCHECKEDCOMPONENT
  
protected:
    nsXULControllers();
    virtual ~nsXULControllers(void);

    void        DeleteControllers();

    nsTArray<nsXULControllerData*>   mControllers;
    PRUint32                         mCurControllerID;
};




#endif 
