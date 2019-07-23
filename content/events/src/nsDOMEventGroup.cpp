





































#include "nsDOMEventGroup.h"

NS_IMPL_ISUPPORTS1(nsDOMEventGroup, nsIDOMEventGroup)

nsDOMEventGroup::nsDOMEventGroup()
{
  
}

nsDOMEventGroup::~nsDOMEventGroup()
{
  
}


NS_IMETHODIMP nsDOMEventGroup::IsSameEventGroup(nsIDOMEventGroup *other, PRBool *retval)
{
  *retval = PR_FALSE;
  if (other == this) {
   *retval = PR_TRUE;
  }
  return NS_OK;
} 

nsresult NS_NewDOMEventGroup(nsIDOMEventGroup** aResult);

nsresult
NS_NewDOMEventGroup(nsIDOMEventGroup** aInstancePtrResult) 
{
  *aInstancePtrResult = new nsDOMEventGroup;
  if (!*aInstancePtrResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aInstancePtrResult);
  return NS_OK;
}
