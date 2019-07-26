





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
  return (mDialogParamBlock == nullptr) ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

nsPKIParamBlock::~nsPKIParamBlock()
{
}


NS_IMETHODIMP 
nsPKIParamBlock::SetNumberStrings( int32_t inNumStrings )
{
  return mDialogParamBlock->SetNumberStrings(inNumStrings);
}

NS_IMETHODIMP 
nsPKIParamBlock::SetInt(int32_t inIndex, int32_t inInt)
{
  return mDialogParamBlock->SetInt(inIndex, inInt);
}

NS_IMETHODIMP 
nsPKIParamBlock::GetInt(int32_t inIndex, int32_t *outInt)
{
  return mDialogParamBlock->GetInt(inIndex, outInt);
}


NS_IMETHODIMP 
nsPKIParamBlock::GetString(int32_t inIndex, PRUnichar **_retval)
{
  return mDialogParamBlock->GetString(inIndex, _retval);
}

NS_IMETHODIMP 
nsPKIParamBlock::SetString(int32_t inIndex, const PRUnichar *inString)
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
nsPKIParamBlock::SetISupportAtIndex(int32_t index, nsISupports *object)
{
  if (!mSupports) {
    mSupports = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
    if (mSupports == nullptr) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  
  mSupports->InsertElementAt(object, index-1);
  return NS_OK;
}


NS_IMETHODIMP 
nsPKIParamBlock::GetISupportAtIndex(int32_t index, nsISupports **_retval)
{
  NS_ENSURE_ARG(_retval);

  *_retval = mSupports->ElementAt(index-1);
  return NS_OK;
}


