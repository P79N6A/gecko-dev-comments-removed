













































#include "nsString.h"

#include "nsIControllers.h"
#include "nsIDOMElement.h"
#include "nsXULControllers.h"
#include "nsString.h"
#include "nsContentUtils.h"



nsXULControllers::nsXULControllers()
: mCurControllerID(0)
{
}

nsXULControllers::~nsXULControllers(void)
{
  DeleteControllers();
}

void
nsXULControllers::DeleteControllers()
{
  PRUint32 count = mControllers.Count();
  for (PRUint32 i = 0; i < count; i++)
  {
    nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.ElementAt(i));
    if (controllerData)
      delete controllerData;    
  }
  
  mControllers.Clear();
}


NS_IMETHODIMP
NS_NewXULControllers(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  NS_PRECONDITION(aOuter == nsnull, "no aggregation");
  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsXULControllers* controllers = new nsXULControllers();
  if (! controllers)
    return NS_ERROR_OUT_OF_MEMORY;
  
  nsresult rv;
  NS_ADDREF(controllers);
  rv = controllers->QueryInterface(aIID, aResult);
  NS_RELEASE(controllers);
  return rv;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULControllers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXULControllers)
  tmp->DeleteControllers();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULControllers)
  {
    PRUint32 i, count = tmp->mControllers.Count();
    for (i = 0; i < count; ++i) {
      nsXULControllerData*  controllerData =
        NS_STATIC_CAST(nsXULControllerData*, tmp->mControllers[i]);
      if (controllerData) {
        cb.NoteXPCOMChild(controllerData->mController);
      }
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN(nsXULControllers)
  NS_INTERFACE_MAP_ENTRY(nsIControllers)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIControllers)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULControllers)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsXULControllers)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXULControllers, nsIControllers)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXULControllers, nsIControllers)

NS_IMETHODIMP
nsXULControllers::GetControllerForCommand(const char *aCommand, nsIController** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  PRUint32 count = mControllers.Count();
  for (PRUint32 i=0; i < count; i++)
  {
    nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.ElementAt(i));
    if (controllerData)
    {
      nsCOMPtr<nsIController> controller;
      controllerData->GetController(getter_AddRefs(controller));
      if (controller)
      {
        PRBool supportsCommand;
        controller->SupportsCommand(aCommand, &supportsCommand);
        if (supportsCommand) {
          *_retval = controller;
          NS_ADDREF(*_retval);
          return NS_OK;
        }
      }
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsXULControllers::InsertControllerAt(PRUint32 aIndex, nsIController *controller)
{
  nsXULControllerData*  controllerData = new nsXULControllerData(++mCurControllerID, controller);
  if (!controllerData) return NS_ERROR_OUT_OF_MEMORY;
#ifdef DEBUG
  PRBool inserted =
#endif
  mControllers.InsertElementAt((void *)controllerData, aIndex);
  NS_ASSERTION(inserted, "Insertion of controller failed");
  return NS_OK;
}

NS_IMETHODIMP
nsXULControllers::RemoveControllerAt(PRUint32 aIndex, nsIController **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.SafeElementAt(aIndex));
  if (!controllerData) return NS_ERROR_FAILURE;

#ifdef DEBUG
  PRBool removed =
#endif
  mControllers.RemoveElementAt(aIndex);
  NS_ASSERTION(removed, "Removal of controller failed");

  controllerData->GetController(_retval);
  delete controllerData;
  
  return NS_OK;
}


NS_IMETHODIMP
nsXULControllers::GetControllerAt(PRUint32 aIndex, nsIController **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.SafeElementAt(aIndex));
  if (!controllerData) return NS_ERROR_FAILURE;

  return controllerData->GetController(_retval);   
}

NS_IMETHODIMP
nsXULControllers::AppendController(nsIController *controller)
{
  
  nsXULControllerData*  controllerData = new nsXULControllerData(++mCurControllerID, controller);
  if (!controllerData) return NS_ERROR_OUT_OF_MEMORY;

#ifdef DEBUG
  PRBool appended =
#endif
  mControllers.AppendElement((void *)controllerData);
  NS_ASSERTION(appended, "Appending controller failed");
  return NS_OK;
}

NS_IMETHODIMP
nsXULControllers::RemoveController(nsIController *controller)
{
  
  nsCOMPtr<nsISupports> controllerSup(do_QueryInterface(controller));
  
  PRUint32 count = mControllers.Count();
  for (PRUint32 i = 0; i < count; i++)
  {
    nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.ElementAt(i));
    if (controllerData)
    {
      nsCOMPtr<nsIController> thisController;
      controllerData->GetController(getter_AddRefs(thisController));
      nsCOMPtr<nsISupports> thisControllerSup(do_QueryInterface(thisController)); 
      if (thisControllerSup == controllerSup)
      {
        mControllers.RemoveElementAt(i);
        delete controllerData;
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;      
}
    

NS_IMETHODIMP
nsXULControllers::GetControllerId(nsIController *controller, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 count = mControllers.Count();
  for (PRUint32 i = 0; i < count; i++)
  {
    nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.ElementAt(i));
    if (controllerData)
    {
      nsCOMPtr<nsIController> thisController;
      controllerData->GetController(getter_AddRefs(thisController));
      if (thisController.get() == controller)
      {
        *_retval = controllerData->GetControllerID();
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;  
}


NS_IMETHODIMP
nsXULControllers::GetControllerById(PRUint32 controllerID, nsIController **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
    
  PRUint32 count = mControllers.Count();
  for (PRUint32 i = 0; i < count; i++)
  {
    nsXULControllerData*  controllerData = NS_STATIC_CAST(nsXULControllerData*, mControllers.ElementAt(i));
    if (controllerData && controllerData->GetControllerID() == controllerID)
    {
      return controllerData->GetController(_retval);
    }
  }
  return NS_ERROR_FAILURE;  
}

NS_IMETHODIMP
nsXULControllers::GetControllerCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mControllers.Count();
  return NS_OK;
}



static char* cloneAllAccess()
{
  static const char allAccess[] = "AllAccess";
  return (char*)nsMemory::Clone(allAccess, sizeof(allAccess));
}

static char* cloneUniversalXPConnect()
{
  static const char universalXPConnect[] = "UniversalXPConnect";
  return (char*)nsMemory::Clone(universalXPConnect, sizeof(universalXPConnect));
}

NS_IMETHODIMP
nsXULControllers::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  *_retval = cloneAllAccess();
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsXULControllers::CanCallMethod(const nsIID * iid, const PRUnichar *methodName,
                                char **_retval)
{
  
  *_retval = cloneUniversalXPConnect();
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsXULControllers::CanGetProperty(const nsIID * iid,
                                 const PRUnichar *propertyName,
                                 char **_retval)
{
  
  *_retval = cloneUniversalXPConnect();
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsXULControllers::CanSetProperty(const nsIID * iid,
                                 const PRUnichar *propertyName,
                                 char **_retval)
{
  
  *_retval = cloneUniversalXPConnect();
  return *_retval ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
