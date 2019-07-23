




































#ifndef nsTransferable_h__
#define nsTransferable_h__

#include "nsIFormatConverter.h"
#include "nsITransferable.h"
#include "nsCOMPtr.h"


class nsVoidArray;
class nsString;
class nsDataObj;
class nsVoidArray;






class nsTransferable : public nsITransferable
{
public:

  nsTransferable();
  virtual ~nsTransferable();

    
  NS_DECL_ISUPPORTS
  NS_DECL_NSITRANSFERABLE

protected:

    
  nsresult GetTransferDataFlavors(nsISupportsArray** aDataFlavorList);
 
  nsVoidArray * mDataArray;
  nsCOMPtr<nsIFormatConverter> mFormatConv;

};

#endif 
