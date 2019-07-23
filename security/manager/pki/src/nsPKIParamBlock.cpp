






































#include "nsPKIParamBlock.h"
#include "nsIServiceManager.h"
#include "nsIDialogParamBlock.h"
#include "nsIMutableArray.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(nsPKIParamBlock, nsIPKIParamBlock,
                                               nsIDialogParamBlock)

nsPKIParamBlock::nsPKIParamBlock()
{
}

nsresult
nsPKIParamBlock::Init()
{
  mDialogParamBlock = do_CreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID);
  return (mDialogParamBlock == nsnull) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

nsPKIParamBlock::~nsPKIParamBlock()
{
}


NS_IMETHODIMP 
nsPKIParamBlock::SetNumberStrings( PRInt32 inNumStrings )
{
  return mDialogParamBlock->SetNumberStrings(inNumStrings);
}

NS_IMETHODIMP 
nsPKIParamBlock::SetInt(PRInt32 inIndex, PRInt32 inInt)
{
  return mDialogParamBlock->SetInt(inIndex, inInt);
}

NS_IMETHODIMP 
nsPKIParamBlock::GetInt(PRInt32 inIndex, PRInt32 *outInt)
{
  return mDialogParamBlock->GetInt(inIndex, outInt);
}


NS_IMETHODIMP 
nsPKIParamBlock::GetString(PRInt32 inIndex, PRUnichar **_retval)
{
  return mDialogParamBlock->GetString(inIndex, _retval);
}

NS_IMETHODIMP 
nsPKIParamBlock::SetString(PRInt32 inIndex, const PRUnichar *inString)
{
  return mDialogParamBlock->SetString(inIndex, inString);
}

NS_IMETHODIMP
nsPKIParamBlock::GetObjects(nsIMutableArray * *aObjects)
{
  return mDialogParamBlock->GetObjects(aObjects);
}

NS_IMETHODIMP
nsPKIParamBlock::SetObjects(nsIMutableArray * aObjects)
{
  return mDialogParamBlock->SetObjects(aObjects);
}




NS_IMETHODIMP 
nsPKIParamBlock::SetISupportAtIndex(PRInt32 index, nsISupports *object)
{
  if (!mSupports) {
    mSupports = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
    if (mSupports == nsnull) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return mSupports->InsertElementAt(object, index-1);
}


NS_IMETHODIMP 
nsPKIParamBlock::GetISupportAtIndex(PRInt32 index, nsISupports **_retval)
{
  NS_ENSURE_ARG(_retval);

  *_retval = mSupports->ElementAt(index-1);
  return NS_OK;
}


