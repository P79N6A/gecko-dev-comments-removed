




































#ifndef nsDataFlavor_h__
#define nsDataFlavor_h__

#include "nsIDataFlavor.h"

class nsISupportsArray;
class nsIDataFlavor;





class nsDataFlavor : public nsIDataFlavor
{

public:
  nsDataFlavor();
  virtual ~nsDataFlavor();

  
  NS_DECL_ISUPPORTS
  
  
  NS_IMETHOD Init(const nsString & aMimeType, 
                  const nsString & aHumanPresentableName);
  NS_IMETHOD GetMimeType(nsString & aMimeStr) const;
  NS_IMETHOD GetHumanPresentableName(nsString & aReadableStr) const;
  NS_IMETHOD Equals(const nsIDataFlavor * aDataFlavor);
  NS_IMETHOD GetPredefinedDataFlavor(nsString & aStr, 
                        nsIDataFlavor ** aDataFlavor);

protected:
  nsString mMimeType;
  nsString mHumanPresentableName;

};

#endif 
