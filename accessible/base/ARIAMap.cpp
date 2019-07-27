






#include "ARIAMap.h"

#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "Role.h"
#include "States.h"

#include "nsAttrName.h"
#include "nsWhitespaceTokenizer.h"

#include "mozilla/BinarySearch.h"

using namespace mozilla;
using namespace mozilla::a11y;
using namespace mozilla::a11y::aria;

static const uint32_t kGenericAccType = 0;
















static nsRoleMapEntry sWAIRoleMaps[] =
{
  { 
    &nsGkAtoms::alert,
    roles::ALERT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::alertdialog,
    roles::DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::application,
    roles::APPLICATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::article,
    roles::DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eReadonlyUntilEditable
  },
  { 
    &nsGkAtoms::button,
    roles::PUSHBUTTON,
    kUseMapRole,
    eNoValue,
    ePressAction,
    eNoLiveAttr,
    eButton,
    kNoReqStates
    
  },
  { 
    &nsGkAtoms::checkbox,
    roles::CHECKBUTTON,
    kUseMapRole,
    eNoValue,
    eCheckUncheckAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIACheckableMixed,
    eARIAReadonly
  },
  { 
    &nsGkAtoms::columnheader,
    roles::COLUMNHEADER,
    kUseMapRole,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    eTableCell,
    kNoReqStates,
    eARIASelectableIfDefined,
    eARIAReadonlyOrEditableIfDefined
  },
  { 
    &nsGkAtoms::combobox,
    roles::COMBOBOX,
    kUseMapRole,
    eNoValue,
    eOpenCloseAction,
    eNoLiveAttr,
    kGenericAccType,
    states::COLLAPSED | states::HASPOPUP | states::VERTICAL,
    eARIAAutoComplete,
    eARIAReadonly,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::dialog,
    roles::DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::directory,
    roles::LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eList,
    kNoReqStates
  },
  { 
    &nsGkAtoms::document,
    roles::DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eReadonlyUntilEditable
  },
  { 
    &nsGkAtoms::form,
    roles::FORM,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::grid,
    roles::TABLE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eSelect | eTable,
    kNoReqStates,
    eARIAMultiSelectable,
    eARIAReadonlyOrEditable,
    eFocusableUntilDisabled
  },
  { 
    &nsGkAtoms::gridcell,
    roles::GRID_CELL,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eTableCell,
    kNoReqStates,
    eARIASelectable,
    eARIAReadonlyOrEditableIfDefined
  },
  { 
    &nsGkAtoms::group,
    roles::GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::heading,
    roles::HEADING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::img,
    roles::GRAPHIC,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::key,
    roles::KEY,
    kUseMapRole,
    eNoValue,
    ePressAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIAPressed
  },
  { 
    &nsGkAtoms::link,
    roles::LINK,
    kUseMapRole,
    eNoValue,
    eJumpAction,
    eNoLiveAttr,
    kGenericAccType,
    states::LINKED
  },
  { 
    &nsGkAtoms::list,
    roles::LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eList,
    states::READONLY
  },
  { 
    &nsGkAtoms::listbox,
    roles::LISTBOX,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eListControl | eSelect,
    states::VERTICAL,
    eARIAMultiSelectable,
    eARIAReadonly,
    eFocusableUntilDisabled,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::listitem,
    roles::LISTITEM,
    kUseMapRole,
    eNoValue,
    eNoAction, 
    eNoLiveAttr,
    kGenericAccType,
    states::READONLY
  },
  { 
    &nsGkAtoms::log_,
    roles::NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::marquee,
    roles::ANIMATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::math,
    roles::FLAT_EQUATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::menu,
    roles::MENUPOPUP,
    kUseMapRole,
    eNoValue,
    eNoAction, 
               
    eNoLiveAttr,
    kGenericAccType,
    states::VERTICAL,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::menubar,
    roles::MENUBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::HORIZONTAL,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::menuitem,
    roles::MENUITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIACheckedMixed
  },
  { 
    &nsGkAtoms::menuitemcheckbox,
    roles::CHECK_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIACheckableMixed
  },
  { 
    &nsGkAtoms::menuitemradio,
    roles::RADIO_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIACheckableBool
  },
  { 
    &nsGkAtoms::note_,
    roles::NOTE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::option,
    roles::OPTION,
    kUseMapRole,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIASelectable,
    eARIACheckedMixed
  },
  { 
    &nsGkAtoms::presentation,
    roles::NOTHING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::progressbar,
    roles::PROGRESSBAR,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::READONLY,
    eIndeterminateIfNoValue
  },
  { 
    &nsGkAtoms::radio,
    roles::RADIOBUTTON,
    kUseMapRole,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIACheckableBool
  },
  { 
    &nsGkAtoms::radiogroup,
    roles::GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::region,
    roles::PANE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::row,
    roles::ROW,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eTableRow,
    kNoReqStates,
    eARIASelectable
  },
  { 
    &nsGkAtoms::rowgroup,
    roles::GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::rowheader,
    roles::ROWHEADER,
    kUseMapRole,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    eTableCell,
    kNoReqStates,
    eARIASelectableIfDefined,
    eARIAReadonlyOrEditableIfDefined
  },
  { 
    &nsGkAtoms::scrollbar,
    roles::SCROLLBAR,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::VERTICAL,
    eARIAOrientation,
    eARIAReadonly
  },
  { 
    &nsGkAtoms::separator_,
    roles::SEPARATOR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::HORIZONTAL,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::slider,
    roles::SLIDER,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::HORIZONTAL,
    eARIAOrientation,
    eARIAReadonly
  },
  { 
    &nsGkAtoms::spinbutton,
    roles::SPINBUTTON,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIAReadonly
  },
  { 
    &nsGkAtoms::status,
    roles::STATUSBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::tab,
    roles::PAGETAB,
    kUseMapRole,
    eNoValue,
    eSwitchAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIASelectable
  },
  { 
    &nsGkAtoms::tablist,
    roles::PAGETABLIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eSelect,
    states::HORIZONTAL,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::tabpanel,
    roles::PROPERTYPAGE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::textbox,
    roles::ENTRY,
    kUseMapRole,
    eNoValue,
    eActivateAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIAAutoComplete,
    eARIAMultiline,
    eARIAReadonlyOrEditable
  },
  { 
    &nsGkAtoms::timer,
    roles::NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates
  },
  { 
    &nsGkAtoms::toolbar,
    roles::TOOLBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    states::HORIZONTAL,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::tooltip,
    roles::TOOLTIP,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates
  },
  { 
    &nsGkAtoms::tree,
    roles::OUTLINE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eSelect,
    states::VERTICAL,
    eARIAReadonly,
    eARIAMultiSelectable,
    eFocusableUntilDisabled,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::treegrid,
    roles::TREE_TABLE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    eSelect | eTable,
    states::VERTICAL,
    eARIAReadonlyOrEditable,
    eARIAMultiSelectable,
    eFocusableUntilDisabled,
    eARIAOrientation
  },
  { 
    &nsGkAtoms::treeitem,
    roles::OUTLINEITEM,
    kUseMapRole,
    eNoValue,
    eActivateAction, 
                     
    eNoLiveAttr,
    kGenericAccType,
    kNoReqStates,
    eARIASelectable,
    eARIACheckedMixed
  }
};

static nsRoleMapEntry sLandmarkRoleMap = {
  &nsGkAtoms::_empty,
  roles::NOTHING,
  kUseNativeRole,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kGenericAccType,
  kNoReqStates
};

nsRoleMapEntry aria::gEmptyRoleMap = {
  &nsGkAtoms::_empty,
  roles::NOTHING,
  kUseMapRole,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kGenericAccType,
  kNoReqStates
};






static const EStateRule sWAIUnivStateMap[] = {
  eARIABusy,
  eARIADisabled,
  eARIAExpanded,  
  eARIAHasPopup,  
  eARIAInvalid,
  eARIARequired,  
  eARIANone
};







struct AttrCharacteristics
{
  nsIAtom** attributeName;
  const uint8_t characteristics;
};

static const AttrCharacteristics gWAIUnivAttrMap[] = {
  {&nsGkAtoms::aria_activedescendant,  ATTR_BYPASSOBJ                               },
  {&nsGkAtoms::aria_atomic,   ATTR_BYPASSOBJ_IF_FALSE | ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_busy,                               ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_checked,           ATTR_BYPASSOBJ | ATTR_VALTOKEN               }, 
  {&nsGkAtoms::aria_controls,          ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_describedby,       ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_disabled,          ATTR_BYPASSOBJ | ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_dropeffect,                         ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_expanded,          ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_flowto,            ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_grabbed,                            ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_haspopup,          ATTR_BYPASSOBJ | ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_hidden,   ATTR_BYPASSOBJ_IF_FALSE | ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_invalid,           ATTR_BYPASSOBJ | ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_label,             ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_labelledby,        ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_level,             ATTR_BYPASSOBJ                               }, 
  {&nsGkAtoms::aria_live,                               ATTR_VALTOKEN | ATTR_GLOBAL },
  {&nsGkAtoms::aria_multiline,         ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_multiselectable,   ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_owns,              ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_orientation,                        ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_posinset,          ATTR_BYPASSOBJ                               }, 
  {&nsGkAtoms::aria_pressed,           ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_readonly,          ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_relevant,          ATTR_BYPASSOBJ                 | ATTR_GLOBAL },
  {&nsGkAtoms::aria_required,          ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_selected,          ATTR_BYPASSOBJ | ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_setsize,           ATTR_BYPASSOBJ                               }, 
  {&nsGkAtoms::aria_sort,                               ATTR_VALTOKEN               },
  {&nsGkAtoms::aria_valuenow,          ATTR_BYPASSOBJ                               },
  {&nsGkAtoms::aria_valuemin,          ATTR_BYPASSOBJ                               },
  {&nsGkAtoms::aria_valuemax,          ATTR_BYPASSOBJ                               },
  {&nsGkAtoms::aria_valuetext,         ATTR_BYPASSOBJ                               }
};

namespace {

struct RoleComparator
{
  const nsDependentSubstring& mRole;
  RoleComparator(const nsDependentSubstring& aRole) : mRole(aRole) {}
  int operator()(const nsRoleMapEntry& aEntry) const {
    return Compare(mRole, aEntry.ARIARoleString());
  }
};

}

nsRoleMapEntry*
aria::GetRoleMap(nsINode* aNode)
{
  nsIContent* content = nsCoreUtils::GetRoleContent(aNode);
  nsAutoString roles;
  if (!content ||
      !content->GetAttr(kNameSpaceID_None, nsGkAtoms::role, roles) ||
      roles.IsEmpty()) {
    
    return nullptr;
  }

  nsWhitespaceTokenizer tokenizer(roles);
  while (tokenizer.hasMoreTokens()) {
    
    const nsDependentSubstring role = tokenizer.nextToken();
    size_t idx;
    if (BinarySearchIf(sWAIRoleMaps, 0, ArrayLength(sWAIRoleMaps),
                       RoleComparator(role), &idx)) {
      return sWAIRoleMaps + idx;
    }
  }

  
  
  return &sLandmarkRoleMap;
}

uint64_t
aria::UniversalStatesFor(mozilla::dom::Element* aElement)
{
  uint64_t state = 0;
  uint32_t index = 0;
  while (MapToState(sWAIUnivStateMap[index], aElement, &state))
    index++;

  return state;
}

uint8_t
aria::AttrCharacteristicsFor(nsIAtom* aAtom)
{
  for (uint32_t i = 0; i < ArrayLength(gWAIUnivAttrMap); i++)
    if (*gWAIUnivAttrMap[i].attributeName == aAtom)
      return gWAIUnivAttrMap[i].characteristics;

  return 0;
}




bool
AttrIterator::Next(nsAString& aAttrName, nsAString& aAttrValue)
{
  while (mAttrIdx < mAttrCount) {
    const nsAttrName* attr = mContent->GetAttrNameAt(mAttrIdx);
    mAttrIdx++;
    if (attr->NamespaceEquals(kNameSpaceID_None)) {
      nsIAtom* attrAtom = attr->Atom();
      nsDependentAtomString attrStr(attrAtom);
      if (!StringBeginsWith(attrStr, NS_LITERAL_STRING("aria-")))
        continue; 

      uint8_t attrFlags = aria::AttrCharacteristicsFor(attrAtom);
      if (attrFlags & ATTR_BYPASSOBJ)
        continue; 

      if ((attrFlags & ATTR_VALTOKEN) &&
           !nsAccUtils::HasDefinedARIAToken(mContent, attrAtom))
        continue; 

      if ((attrFlags & ATTR_BYPASSOBJ_IF_FALSE) &&
          mContent->AttrValueIs(kNameSpaceID_None, attrAtom,
                                nsGkAtoms::_false, eCaseMatters)) {
        continue; 
      }

      nsAutoString value;
      if (mContent->GetAttr(kNameSpaceID_None, attrAtom, value)) {
        aAttrName.Assign(Substring(attrStr, 5));
        aAttrValue.Assign(value);
        return true;
      }
    }
  }

  return false;
}

