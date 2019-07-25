











































#ifndef nsGridCell_h___
#define nsGridCell_h___

#include "nsIFrame.h"

class nsBoxLayoutState;
struct nsSize;









class nsGridCell
{
public:
    nsGridCell();
    virtual ~nsGridCell();

    nsSize      GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMinSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
    PRBool      IsCollapsed(nsBoxLayoutState& aBoxLayoutState);


    nsIBox*     GetBoxInColumn()             { return mBoxInColumn; }
    nsIBox*     GetBoxInRow()                { return mBoxInRow; }
    void        SetBoxInRow(nsIBox* aBox)    { mBoxInRow = aBox; }
    void        SetBoxInColumn(nsIBox* aBox) { mBoxInColumn = aBox; }

private:
    nsIBox* mBoxInColumn;
    nsIBox* mBoxInRow;
};

#endif

