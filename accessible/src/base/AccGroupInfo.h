




































#ifndef AccGroupInfo_h_
#define AccGroupInfo_h_

#include "nsAccessible.h"
#include "nsAccUtils.h"




class AccGroupInfo
{
public:
  AccGroupInfo(nsAccessible* aItem, PRUint32 aRole);
  ~AccGroupInfo() { }

  PRInt32 PosInSet() const { return mPosInSet; }
  PRUint32 SetSize() const { return mSetSize; }
  nsAccessible* GetConceptualParent() const { return mParent; }

  


  static AccGroupInfo* CreateGroupInfo(nsAccessible* aAccessible)
  {
    PRUint32 role = nsAccUtils::Role(aAccessible);
    if (role != nsIAccessibleRole::ROLE_ROW &&
        role != nsIAccessibleRole::ROLE_GRID_CELL &&
        role != nsIAccessibleRole::ROLE_OUTLINEITEM &&
        role != nsIAccessibleRole::ROLE_OPTION &&
        role != nsIAccessibleRole::ROLE_LISTITEM &&
        role != nsIAccessibleRole::ROLE_MENUITEM &&
        role != nsIAccessibleRole::ROLE_CHECK_MENU_ITEM &&
        role != nsIAccessibleRole::ROLE_RADIO_MENU_ITEM &&
        role != nsIAccessibleRole::ROLE_RADIOBUTTON &&
        role != nsIAccessibleRole::ROLE_PAGETAB)
      return nsnull;

    AccGroupInfo* info = new AccGroupInfo(aAccessible, BaseRole(role));
    return info;
  }

private:
  AccGroupInfo(const AccGroupInfo&);
  AccGroupInfo& operator =(const AccGroupInfo&);

  static PRUint32 BaseRole(PRUint32 aRole)
  {
    if (aRole == nsIAccessibleRole::ROLE_CHECK_MENU_ITEM ||
        aRole == nsIAccessibleRole::ROLE_RADIO_MENU_ITEM)
      return nsIAccessibleRole::ROLE_MENUITEM;
    return aRole;
  }

  PRUint32 mPosInSet;
  PRUint32 mSetSize;
  nsAccessible* mParent;
};

#endif
