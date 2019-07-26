












#ifndef nsStackLayout_h___
#define nsStackLayout_h___

#include "mozilla/Attributes.h"
#include "nsBoxLayout.h"
#include "nsCOMPtr.h"
#include "nsCoord.h"

class nsIPresShell;

nsresult NS_NewStackLayout(nsIPresShell* aPresShell, nsCOMPtr<nsBoxLayout>& aNewLayout);

class nsStackLayout : public nsBoxLayout
{
public:

  friend nsresult NS_NewStackLayout(nsIPresShell* aPresShell, nsCOMPtr<nsBoxLayout>& aNewLayout);
  static void Shutdown();

  nsStackLayout();

  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aState);

  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nscoord GetAscent(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  
  
  
  
  static uint8_t GetOffset(nsBoxLayoutState& aState, nsIFrame* aChild, nsMargin& aMargin);

private:
  static nsBoxLayout* gInstance;

}; 



#endif

