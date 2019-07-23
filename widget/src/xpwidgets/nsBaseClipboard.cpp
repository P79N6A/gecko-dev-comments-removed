




































#include "nsBaseClipboard.h"

#include "nsIClipboardOwner.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"


nsBaseClipboard::nsBaseClipboard()
{
  mClipboardOwner          = nsnull;
  mTransferable            = nsnull;
  mIgnoreEmptyNotification = PR_FALSE;

}

nsBaseClipboard::~nsBaseClipboard()
{
  EmptyClipboard(kSelectionClipboard);
  EmptyClipboard(kGlobalClipboard);
}

NS_IMPL_ISUPPORTS1(nsBaseClipboard, nsIClipboard)





NS_IMETHODIMP nsBaseClipboard::SetData(nsITransferable * aTransferable, nsIClipboardOwner * anOwner,
                                        PRInt32 aWhichClipboard)
{
  NS_ASSERTION ( aTransferable, "clipboard given a null transferable" );

  if (aTransferable == mTransferable && anOwner == mClipboardOwner)
    return NS_OK;
  PRBool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  if ( !selectClipPresent && aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  EmptyClipboard(aWhichClipboard);

  mClipboardOwner = anOwner;
  if ( anOwner )
    NS_ADDREF(mClipboardOwner);

  mTransferable = aTransferable;
  
  nsresult rv = NS_ERROR_FAILURE;

  if ( mTransferable ) {
    NS_ADDREF(mTransferable);
    rv = SetNativeClipboardData(aWhichClipboard);
  }
  
  return rv;
}





NS_IMETHODIMP nsBaseClipboard::GetData(nsITransferable * aTransferable, PRInt32 aWhichClipboard)
{
  NS_ASSERTION ( aTransferable, "clipboard given a null transferable" );
  
  PRBool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  if ( !selectClipPresent && aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  if ( aTransferable )
    return GetNativeClipboardData(aTransferable, aWhichClipboard);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsBaseClipboard::EmptyClipboard(PRInt32 aWhichClipboard)
{
  PRBool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  if ( !selectClipPresent && aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  if (mIgnoreEmptyNotification)
    return NS_OK;

  if ( mClipboardOwner ) {
    mClipboardOwner->LosingOwnership(mTransferable);
    NS_RELEASE(mClipboardOwner);
  }

  NS_IF_RELEASE(mTransferable);

  return NS_OK;
}

NS_IMETHODIMP
nsBaseClipboard::HasDataMatchingFlavors(nsISupportsArray* aFlavorList,
                                        PRInt32 aWhichClipboard,
                                        PRBool* outResult) 
{
  *outResult = PR_TRUE;  
  return NS_OK;
}

NS_IMETHODIMP
nsBaseClipboard::SupportsSelectionClipboard(PRBool* _retval)
{
  *_retval = PR_FALSE;   
  return NS_OK;
}
