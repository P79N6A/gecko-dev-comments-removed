













































#ifndef nsStackLayout_h___
#define nsStackLayout_h___

#include "nsBoxLayout.h"
#include "nsCOMPtr.h"

nsresult NS_NewStackLayout(nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout);

class nsStackLayout : public nsBoxLayout
{
public:

  friend nsresult NS_NewStackLayout(nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout);
  static void Shutdown();

  nsStackLayout();

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);

  
  
  
  
  static PRUint8 GetOffset(nsBoxLayoutState& aState, nsIBox* aChild, nsMargin& aMargin);

private:
  static nsIBoxLayout* gInstance;

}; 



#endif

