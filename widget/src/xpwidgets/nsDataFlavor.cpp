





































#include "nsDataFlavor.h"
#include "nsReadableUtils.h"

static NS_DEFINE_IID(kIDataFlavor, NS_IDATAFLAVOR_IID);

NS_IMPL_ADDREF(nsDataFlavor)
NS_IMPL_RELEASE(nsDataFlavor)






nsDataFlavor::nsDataFlavor()
{
}






nsDataFlavor::~nsDataFlavor()
{
}






 
nsresult nsDataFlavor::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{

  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv = NS_NOINTERFACE;

  if (aIID.Equals(kIDataFlavor)) {
    *aInstancePtr = (void*) ((nsIDataFlavor*)this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return rv;
}





NS_METHOD nsDataFlavor::Init(const nsString & aMimeType, const nsString & aHumanPresentableName)
{
  mMimeType = aMimeType;
  mHumanPresentableName = aHumanPresentableName;

  char * str = ToNewCString(mMimeType);

  delete[] str;

  return NS_OK;
}





NS_METHOD nsDataFlavor::GetMimeType(nsString & aMimeStr) const
{
  aMimeStr = mMimeType;
  return NS_OK;
}





NS_METHOD nsDataFlavor::GetHumanPresentableName(nsString & aHumanPresentableName) const
{
  aHumanPresentableName = mHumanPresentableName;
  return NS_OK;
}





NS_METHOD nsDataFlavor::Equals(const nsIDataFlavor * aDataFlavor)
{
  nsString mimeInQues;
  aDataFlavor->GetMimeType(mimeInQues);

  return (mMimeType.Equals(mimeInQues)?NS_OK:NS_ERROR_FAILURE);
}








NS_METHOD nsDataFlavor::GetPredefinedDataFlavor(nsString & aStr, 
                                                nsIDataFlavor ** aDataFlavor)
{
  return NS_OK;
}
