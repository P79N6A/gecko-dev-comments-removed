











#ifndef nsGridCell_h___
#define nsGridCell_h___

class nsBoxLayoutState;
struct nsSize;
class nsIFrame;









class nsGridCell
{
public:
    nsGridCell();
    virtual ~nsGridCell();

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

