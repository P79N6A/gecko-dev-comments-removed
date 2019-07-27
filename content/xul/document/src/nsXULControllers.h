










#ifndef nsXULControllers_h__
#define nsXULControllers_h__

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsWeakPtr.h"
#include "nsIControllers.h"
#include "nsCycleCollectionParticipant.h"


class nsXULControllerData
{
public:
                            nsXULControllerData(uint32_t inControllerID, nsIController* inController)
                            : mControllerID(inControllerID)
                            , mController(inController)
                            {                            
                            }

                            ~nsXULControllerData() {}

    uint32_t                GetControllerID()   { return mControllerID; }

    nsresult                GetController(nsIController **outController)
                            {
                              NS_IF_ADDREF(*outController = mController);
                              return NS_OK;
                            }
    
    uint32_t                mControllerID;
    nsCOMPtr<nsIController> mController;
};


nsresult NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);

class nsXULControllers : public nsIControllers
{
public:
    friend nsresult
    NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult);

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULControllers, nsIControllers)
    NS_DECL_NSICONTROLLERS
  
protected:
    nsXULControllers();
    virtual ~nsXULControllers(void);

    void        DeleteControllers();

    nsTArray<nsXULControllerData*>   mControllers;
    uint32_t                         mCurControllerID;
};




#endif 
