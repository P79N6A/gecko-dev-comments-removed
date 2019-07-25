




































#ifndef nsAccIterator_h_
#define nsAccIterator_h_

#include "nsAccessible.h"
#include "nsAccUtils.h"




typedef PRBool (*AccIteratorFilterFuncPtr) (nsAccessible *);





class nsAccIterator
{
public:
  


  enum IterationType {
    


    eFlatNav,

    



    eTreeNav
  };

  nsAccIterator(nsAccessible *aRoot, AccIteratorFilterFuncPtr aFilterFunc,
                IterationType aIterationType = eFlatNav);
  ~nsAccIterator();

  



  nsAccessible *GetNext();

  


  static PRBool GetSelected(nsAccessible *aAccessible)
  {
    return nsAccUtils::State(aAccessible) & nsIAccessibleStates::STATE_SELECTED;
  }
  static PRBool GetSelectable(nsAccessible *aAccessible)
  {
    return nsAccUtils::State(aAccessible) & nsIAccessibleStates::STATE_SELECTABLE;
  }
  static PRBool GetRow(nsAccessible *aAccessible)
  {
    return nsAccUtils::Role(aAccessible) == nsIAccessibleRole::ROLE_ROW;
  }
  static PRBool GetCell(nsAccessible *aAccessible)
  {
    PRUint32 role = nsAccUtils::Role(aAccessible);
    return role == nsIAccessibleRole::ROLE_GRID_CELL ||
           role == nsIAccessibleRole::ROLE_ROWHEADER ||
           role == nsIAccessibleRole::ROLE_COLUMNHEADER;
  }

private:
  nsAccIterator();
  nsAccIterator(const nsAccIterator&);
  nsAccIterator& operator =(const nsAccIterator&);

  struct IteratorState
  {
    IteratorState(nsAccessible *aParent, IteratorState *mParentState = nsnull);

    nsAccessible *mParent;
    PRInt32 mIndex;
    IteratorState *mParentState;
  };

  AccIteratorFilterFuncPtr mFilterFunc;
  PRBool mIsDeep;
  IteratorState *mState;
};

#endif
