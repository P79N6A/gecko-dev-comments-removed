











#ifndef nsGridCell_h___
#define nsGridCell_h___

#include "mozilla/Attributes.h"

class nsBoxLayoutState;
struct nsSize;
class nsIFrame;









class nsGridCell MOZ_FINAL
{
public:
    nsGridCell();
    ~nsGridCell();

    nsSize      GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMinSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
    bool        IsCollapsed();


    nsIFrame*   GetBoxInColumn()               { return mBoxInColumn; }
    nsIFrame*   GetBoxInRow()                  { return mBoxInRow; }
    void        SetBoxInRow(nsIFrame* aBox)    { mBoxInRow = aBox; }
    void        SetBoxInColumn(nsIFrame* aBox) { mBoxInColumn = aBox; }

private:
    nsIFrame* mBoxInColumn;
    nsIFrame* mBoxInRow;
};

#endif

