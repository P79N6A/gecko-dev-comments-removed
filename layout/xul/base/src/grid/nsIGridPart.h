




#ifndef nsIGridPart_h___
#define nsIGridPart_h___

#include "nsISupports.h"

class nsGridRowGroupLayout;
class nsGrid;
class nsGridRowLayout;
class nsGridRow;
class nsGridLayout2;


#define NS_IGRIDPART_IID \
{ 0x07373ed7, 0xe947, 0x4a5e, \
  { 0xb3, 0x6c, 0x69, 0xf7, 0xc1, 0x95, 0x67, 0x7b } }





class nsIGridPart : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGRIDPART_IID)

  virtual nsGridRowGroupLayout* CastToRowGroupLayout()=0;
  virtual nsGridLayout2* CastToGridLayout()=0;

  
















  virtual nsGrid* GetGrid(nsIFrame* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor=nullptr)=0;

  







  virtual nsIGridPart* GetParentGridPart(nsIFrame* aBox, nsIFrame** aParentBox) = 0;

  





  virtual void CountRowsColumns(nsIFrame* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount)=0;
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState)=0;
  virtual PRInt32 BuildRows(nsIFrame* aBox, nsGridRow* aRows)=0;
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal)=0;
  virtual PRInt32 GetRowCount() { return 1; }
  
  


  enum Type { eGrid, eRowGroup, eRowLeaf };
  virtual Type GetType()=0;

  


  bool CanContain(nsIGridPart* aPossibleChild) {
    Type thisType = GetType(), childType = aPossibleChild->GetType();
    return thisType + 1 == childType || (thisType == eRowGroup && childType == eRowGroup);
  }

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGridPart, NS_IGRIDPART_IID)

#endif

