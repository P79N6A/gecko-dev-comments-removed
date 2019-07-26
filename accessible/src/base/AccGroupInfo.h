



#ifndef AccGroupInfo_h_
#define AccGroupInfo_h_

#include "Accessible-inl.h"
#include "nsAccUtils.h"

namespace mozilla {
namespace a11y {




class AccGroupInfo
{
public:
  ~AccGroupInfo() { MOZ_COUNT_DTOR(AccGroupInfo); }

  


  uint32_t PosInSet() const { return mPosInSet; }

  


  uint32_t SetSize() const { return mSetSize; }

  



  Accessible* ConceptualParent() const { return mParent; }

  


  static AccGroupInfo* CreateGroupInfo(Accessible* aAccessible)
  {
    mozilla::a11y::role role = aAccessible->Role();
    if (role != mozilla::a11y::roles::ROW &&
        role != mozilla::a11y::roles::GRID_CELL &&
        role != mozilla::a11y::roles::OUTLINEITEM &&
        role != mozilla::a11y::roles::OPTION &&
        role != mozilla::a11y::roles::LISTITEM &&
        role != mozilla::a11y::roles::MENUITEM &&
        role != mozilla::a11y::roles::COMBOBOX_OPTION &&
        role != mozilla::a11y::roles::RICH_OPTION &&
        role != mozilla::a11y::roles::CHECK_RICH_OPTION &&
        role != mozilla::a11y::roles::PARENT_MENUITEM &&
        role != mozilla::a11y::roles::CHECK_MENU_ITEM &&
        role != mozilla::a11y::roles::RADIO_MENU_ITEM &&
        role != mozilla::a11y::roles::RADIOBUTTON &&
        role != mozilla::a11y::roles::PAGETAB)
      return nullptr;

    AccGroupInfo* info = new AccGroupInfo(aAccessible, BaseRole(role));
    return info;
  }

  


  static Accessible* FirstItemOf(Accessible* aContainer);

  


  static Accessible* NextItemTo(Accessible* aItem);

protected:
  AccGroupInfo(Accessible* aItem, a11y::role aRole);

private:
  AccGroupInfo() MOZ_DELETE;
  AccGroupInfo(const AccGroupInfo&) MOZ_DELETE;
  AccGroupInfo& operator =(const AccGroupInfo&) MOZ_DELETE;

  static mozilla::a11y::role BaseRole(mozilla::a11y::role aRole)
  {
    if (aRole == mozilla::a11y::roles::CHECK_MENU_ITEM ||
        aRole == mozilla::a11y::roles::PARENT_MENUITEM ||
        aRole == mozilla::a11y::roles::RADIO_MENU_ITEM)
      return mozilla::a11y::roles::MENUITEM;

    if (aRole == mozilla::a11y::roles::CHECK_RICH_OPTION)
      return mozilla::a11y::roles::RICH_OPTION;

    return aRole;
  }

  



  static bool IsConceptualParent(a11y::role aRole, a11y::role aParentRole);

  uint32_t mPosInSet;
  uint32_t mSetSize;
  Accessible* mParent;
};

} 
} 

#endif
