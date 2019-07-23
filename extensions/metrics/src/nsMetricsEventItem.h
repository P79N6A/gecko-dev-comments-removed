





































#ifndef nsMetricsEventItem_h__
#define nsMetricsEventItem_h__

#include "nsIMetricsService.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"

class nsIPropertyBag;



class nsMetricsEventItem : public nsIMetricsEventItem
{
 public:
  nsMetricsEventItem(const nsAString &itemNamespace,
                     const nsAString &itemName);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSEVENTITEM

 private:
  ~nsMetricsEventItem();

  nsString mNamespace;
  nsString mName;
  nsCOMPtr<nsIPropertyBag> mProperties;
  nsTArray< nsCOMPtr<nsIMetricsEventItem> > mChildren;
};

#endif  
