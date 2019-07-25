











#ifndef nsGridCell_h___
#define nsGridCell_h___

class nsBoxLayoutState;
struct nsSize;
class nsIFrame;
typedef class nsIFrame nsIBox;









class nsGridCell
{
public:
    nsGridCell();
    virtual ~nsGridCell();

    nsSize      GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMinSize(nsBoxLayoutState& aBoxLayoutState);
    nsSize      GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
    bool        IsCollapsed();


    nsIBox*     GetBoxInColumn()             { return mBoxInColumn; }
    nsIBox*     GetBoxInRow()                { return mBoxInRow; }
    void        SetBoxInRow(nsIBox* aBox)    { mBoxInRow = aBox; }
    void        SetBoxInColumn(nsIBox* aBox) { mBoxInColumn = aBox; }

private:
    nsIBox* mBoxInColumn;
    nsIBox* mBoxInRow;
};

#endif

