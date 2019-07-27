




#include "nsBaseClipboard.h"

#include "nsIClipboardOwner.h"
#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"

nsBaseClipboard::nsBaseClipboard()
{
  mClipboardOwner          = nullptr;
  mTransferable            = nullptr;
  mIgnoreEmptyNotification = false;
  mEmptyingForSetData      = false;
}

nsBaseClipboard::~nsBaseClipboard()
{
  EmptyClipboard(kSelectionClipboard);
  EmptyClipboard(kGlobalClipboard);
  EmptyClipboard(kFindClipboard);
}

NS_IMPL_ISUPPORTS(nsBaseClipboard, nsIClipboard)





NS_IMETHODIMP nsBaseClipboard::SetData(nsITransferable * aTransferable, nsIClipboardOwner * anOwner,
                                        int32_t aWhichClipboard)
{
  NS_ASSERTION ( aTransferable, "clipboard given a null transferable" );

  if (aTransferable == mTransferable && anOwner == mClipboardOwner)
    return NS_OK;
  bool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  bool findClipPresent;
  SupportsFindClipboard(&findClipPresent);
  if ( !selectClipPresent && !findClipPresent && aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  mEmptyingForSetData = true;
  EmptyClipboard(aWhichClipboard);
  mEmptyingForSetData = false;

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





NS_IMETHODIMP nsBaseClipboard::GetData(nsITransferable * aTransferable, int32_t aWhichClipboard)
{
  NS_ASSERTION ( aTransferable, "clipboard given a null transferable" );

  bool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  bool findClipPresent;
  SupportsFindClipboard(&findClipPresent);
  if ( !selectClipPresent && !findClipPresent && aWhichClipboard != kGlobalClipboard )
    return NS_ERROR_FAILURE;

  if ( aTransferable )
    return GetNativeClipboardData(aTransferable, aWhichClipboard);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsBaseClipboard::EmptyClipboard(int32_t aWhichClipboard)
{
  bool selectClipPresent;
  SupportsSelectionClipboard(&selectClipPresent);
  bool findClipPresent;
  SupportsFindClipboard(&findClipPresent);
  if ( !selectClipPresent && !findClipPresent && aWhichClipboard != kGlobalClipboard )
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
nsBaseClipboard::HasDataMatchingFlavors(const char** aFlavorList,
                                        uint32_t aLength,
                                        int32_t aWhichClipboard,
                                        bool* outResult) 
{
  *outResult = true;  
  return NS_OK;
}

NS_IMETHODIMP
nsBaseClipboard::SupportsSelectionClipboard(bool* _retval)
{
  *_retval = false;   
  return NS_OK;
}

NS_IMETHODIMP
nsBaseClipboard::SupportsFindClipboard(bool* _retval)
{
  *_retval = false;   
  return NS_OK;
}
