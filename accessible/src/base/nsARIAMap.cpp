






































#include "nsARIAMap.h"

#include "nsIAccessibleRole.h"
#include "Role.h"
#include "States.h"

#include "nsIContent.h"

using namespace mozilla::a11y;
















nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {
    "alert",
    roles::ALERT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "alertdialog",
    roles::DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "application",
    roles::APPLICATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "article",
    roles::DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eReadonlyUntilEditable
  },
  {
    "button",
    roles::PUSHBUTTON,
    kUseMapRole,
    eNoValue,
    ePressAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAPressed
  },
  {
    "checkbox",
    roles::CHECKBUTTON,
    kUseMapRole,
    eNoValue,
    eCheckUncheckAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableMixed,
    eARIAReadonly
  },
  {
    "columnheader",
    roles::COLUMNHEADER,
    kUseMapRole,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable,
    eARIAReadonly
  },
  {
    "combobox",
    roles::COMBOBOX,
    kUseMapRole,
    eNoValue,
    eOpenCloseAction,
    eNoLiveAttr,
    states::COLLAPSED | states::HASPOPUP,
    eARIAAutoComplete,
    eARIAReadonly
  },
  {
    "dialog",
    roles::DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "directory",
    roles::LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "document",
    roles::DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eReadonlyUntilEditable
  },
  {
    "grid",
    roles::TABLE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    states::FOCUSABLE,
    eARIAMultiSelectable,
    eARIAReadonly
  },
  {
    "gridcell",
    roles::GRID_CELL,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable,
    eARIAReadonly
  },
  {
    "group",
    roles::GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "heading",
    roles::HEADING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "img",
    roles::GRAPHIC,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "link",
    roles::LINK,
    kUseMapRole,
    eNoValue,
    eJumpAction,
    eNoLiveAttr,
    states::LINKED
  },
  {
    "list",
    roles::LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    states::READONLY
  },
  {
    "listbox",
    roles::LISTBOX,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAMultiSelectable,
    eARIAReadonly
  },
  {
    "listitem",
    roles::LISTITEM,
    kUseMapRole,
    eNoValue,
    eNoAction, 
    eNoLiveAttr,
    states::READONLY
  },
  {
    "log",
    roles::NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "marquee",
    roles::ANIMATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates
  },
  {
    "math",
    roles::FLAT_EQUATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menu",
    roles::MENUPOPUP,
    kUseMapRole,
    eNoValue,
    eNoAction, 
               
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menubar",
    roles::MENUBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menuitem",
    roles::MENUITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckedMixed
  },
  {
    "menuitemcheckbox",
    roles::CHECK_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableMixed
  },
  {
    "menuitemradio",
    roles::RADIO_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableBool
  },
  {
    "option",
    roles::OPTION,
    kUseMapRole,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable,
    eARIACheckedMixed
  },
  {
    "presentation",
    roles::NOTHING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "progressbar",
    roles::PROGRESSBAR,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    states::READONLY
  },
  {
    "radio",
    roles::RADIOBUTTON,
    kUseMapRole,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableBool
  },
  {
    "radiogroup",
    roles::GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "region",
    roles::PANE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "row",
    roles::ROW,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable
  },
  {
    "rowheader",
    roles::ROWHEADER,
    kUseMapRole,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable,
    eARIAReadonly
  },
  {
    "scrollbar",
    roles::SCROLLBAR,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAOrientation,
    eARIAReadonly
  },
  {
    "separator",
    roles::SEPARATOR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAOrientation
  },
  {
    "slider",
    roles::SLIDER,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAOrientation,
    eARIAReadonly
  },
  {
    "spinbutton",
    roles::SPINBUTTON,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAReadonly
  },
  {
    "status",
    roles::STATUSBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "tab",
    roles::PAGETAB,
    kUseMapRole,
    eNoValue,
    eSwitchAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable
  },
  {
    "tablist",
    roles::PAGETABLIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "tabpanel",
    roles::PROPERTYPAGE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "textbox",
    roles::ENTRY,
    kUseMapRole,
    eNoValue,
    eActivateAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAAutoComplete,
    eARIAMultiline,
    eARIAReadonlyOrEditable
  },
  {
    "timer",
    roles::NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates
  },
  {
    "toolbar",
    roles::TOOLBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "tooltip",
    roles::TOOLTIP,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "tree",
    roles::OUTLINE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAReadonly,
    eARIAMultiSelectable
  },
  {
    "treegrid",
    roles::TREE_TABLE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAReadonly,
    eARIAMultiSelectable
  },
  {
    "treeitem",
    roles::OUTLINEITEM,
    kUseMapRole,
    eNoValue,
    eActivateAction, 
                     
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable,
    eARIACheckedMixed
  }
};

PRUint32 nsARIAMap::gWAIRoleMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIRoleMap);

nsRoleMapEntry nsARIAMap::gLandmarkRoleMap = {
  "",
  roles::NOTHING,
  kUseNativeRole,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kNoReqStates
};

nsRoleMapEntry nsARIAMap::gEmptyRoleMap = {
  "",
  roles::NOTHING,
  kUseMapRole,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kNoReqStates
};

nsStateMapEntry nsARIAMap::gWAIStateMap[] = {
  
  nsStateMapEntry(),

  
  nsStateMapEntry(&nsGkAtoms::aria_autocomplete,
                  "inline", states::SUPPORTS_AUTOCOMPLETION,
                  "list", states::HASPOPUP | states::SUPPORTS_AUTOCOMPLETION,
                  "both", states::HASPOPUP | states::SUPPORTS_AUTOCOMPLETION),

  
  nsStateMapEntry(&nsGkAtoms::aria_busy,
                  "true", states::BUSY,
                  "error", states::INVALID),

  
  nsStateMapEntry(&nsGkAtoms::aria_checked, kBoolType,
                  states::CHECKABLE, states::CHECKED, 0, true),

  
  nsStateMapEntry(&nsGkAtoms::aria_checked, kMixedType,
                  states::CHECKABLE, states::CHECKED, 0, true),

  
  nsStateMapEntry(&nsGkAtoms::aria_checked, kMixedType,
                  states::CHECKABLE, states::CHECKED, 0),

  
  nsStateMapEntry(&nsGkAtoms::aria_disabled, kBoolType,
                  0, states::UNAVAILABLE),

  
  nsStateMapEntry(&nsGkAtoms::aria_expanded, kBoolType,
                  0, states::EXPANDED, states::COLLAPSED),

  
  nsStateMapEntry(&nsGkAtoms::aria_haspopup, kBoolType,
                  0, states::HASPOPUP),

  
  nsStateMapEntry(&nsGkAtoms::aria_invalid, kBoolType,
                  0, states::INVALID),

  
  nsStateMapEntry(&nsGkAtoms::aria_multiline, kBoolType,
                  0, states::MULTI_LINE, states::SINGLE_LINE, true),

  
  nsStateMapEntry(&nsGkAtoms::aria_multiselectable, kBoolType,
                  0, states::MULTISELECTABLE | states::EXTSELECTABLE),

  
  nsStateMapEntry(&nsGkAtoms::aria_orientation, eUseFirstState,
                  "horizontal", states::HORIZONTAL,
                  "vertical", states::VERTICAL),

  
  nsStateMapEntry(&nsGkAtoms::aria_pressed, kMixedType,
                  states::CHECKABLE, states::PRESSED),

  
  nsStateMapEntry(&nsGkAtoms::aria_readonly, kBoolType,
                  0, states::READONLY),

  
  nsStateMapEntry(&nsGkAtoms::aria_readonly, kBoolType,
                  0, states::READONLY, states::EDITABLE, true),

  
  nsStateMapEntry(&nsGkAtoms::aria_required, kBoolType,
                  0, states::REQUIRED),

  
  nsStateMapEntry(&nsGkAtoms::aria_selected, kBoolType,
                  states::SELECTABLE, states::SELECTED, 0, true),

  
  nsStateMapEntry(states::READONLY, states::EDITABLE)
};






eStateMapEntryID nsARIAMap::gWAIUnivStateMap[] = {
  eARIABusy,
  eARIADisabled,
  eARIAExpanded,  
  eARIAHasPopup,  
  eARIAInvalid,
  eARIARequired,  
  eARIANone
};







nsAttributeCharacteristics nsARIAMap::gWAIUnivAttrMap[] = {
  {&nsGkAtoms::aria_activedescendant,  ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_atomic,                             ATTR_VALTOKEN },
  {&nsGkAtoms::aria_busy,                               ATTR_VALTOKEN },
  {&nsGkAtoms::aria_checked,           ATTR_BYPASSOBJ | ATTR_VALTOKEN }, 
  {&nsGkAtoms::aria_controls,          ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_describedby,       ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_disabled,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_dropeffect,                         ATTR_VALTOKEN },
  {&nsGkAtoms::aria_expanded,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_flowto,            ATTR_BYPASSOBJ                 },  
  {&nsGkAtoms::aria_grabbed,                            ATTR_VALTOKEN },
  {&nsGkAtoms::aria_haspopup,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_hidden,                             ATTR_VALTOKEN },
  {&nsGkAtoms::aria_invalid,           ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_label,             ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_labelledby,        ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_level,             ATTR_BYPASSOBJ                 }, 
  {&nsGkAtoms::aria_live,                               ATTR_VALTOKEN },
  {&nsGkAtoms::aria_multiline,         ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_multiselectable,   ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_owns,              ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_orientation,                        ATTR_VALTOKEN },
  {&nsGkAtoms::aria_posinset,          ATTR_BYPASSOBJ                 }, 
  {&nsGkAtoms::aria_pressed,           ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_readonly,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_relevant,          ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_required,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_selected,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsGkAtoms::aria_setsize,           ATTR_BYPASSOBJ                 }, 
  {&nsGkAtoms::aria_sort,                               ATTR_VALTOKEN },
  {&nsGkAtoms::aria_valuenow,          ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_valuemin,          ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_valuemax,          ATTR_BYPASSOBJ                 },
  {&nsGkAtoms::aria_valuetext,         ATTR_BYPASSOBJ                 }
};

PRUint32 nsARIAMap::gWAIUnivAttrMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIUnivAttrMap);





nsStateMapEntry::nsStateMapEntry() :
  mAttributeName(nsnull),
  mIsToken(false),
  mPermanentState(0),
  mValue1(nsnull),
  mState1(0),
  mValue2(nsnull),
  mState2(0),
  mValue3(nsnull),
  mState3(0),
  mDefaultState(0),
  mDefinedIfAbsent(false)
{}

nsStateMapEntry::nsStateMapEntry(PRUint64 aDefaultState,
                                 PRUint64 aExclusingState) :
  mAttributeName(nsnull),
  mIsToken(false),
  mPermanentState(0),
  mValue1(nsnull),
  mState1(0),
  mValue2(nsnull),
  mState2(0),
  mValue3(nsnull),
  mState3(0),
  mDefaultState(aDefaultState),
  mDefinedIfAbsent(false),
  mExcludingState(aExclusingState)
{
}

nsStateMapEntry::nsStateMapEntry(nsIAtom** aAttrName, eStateValueType aType,
                                 PRUint64 aPermanentState,
                                 PRUint64 aTrueState,
                                 PRUint64 aFalseState,
                                 bool aDefinedIfAbsent) :
  mAttributeName(aAttrName),
  mIsToken(true),
  mPermanentState(aPermanentState),
  mValue1("false"),
  mState1(aFalseState),
  mValue2(nsnull),
  mState2(0),
  mValue3(nsnull),
  mState3(0),
  mDefaultState(aTrueState),
  mDefinedIfAbsent(aDefinedIfAbsent),
  mExcludingState(0)
{
  if (aType == kMixedType) {
    mValue2 = "mixed";
    mState2 = states::MIXED;
  }
}

nsStateMapEntry::nsStateMapEntry(nsIAtom** aAttrName,
                                 const char* aValue1, PRUint64 aState1,
                                 const char* aValue2, PRUint64 aState2,
                                 const char* aValue3, PRUint64 aState3) :
  mAttributeName(aAttrName), mIsToken(false), mPermanentState(0),
  mValue1(aValue1), mState1(aState1),
  mValue2(aValue2), mState2(aState2),
  mValue3(aValue3), mState3(aState3),
  mDefaultState(0), mDefinedIfAbsent(false), mExcludingState(0)
{
}

nsStateMapEntry::nsStateMapEntry(nsIAtom** aAttrName,
                                 EDefaultStateRule aDefaultStateRule,
                                 const char* aValue1, PRUint64 aState1,
                                 const char* aValue2, PRUint64 aState2,
                                 const char* aValue3, PRUint64 aState3) :
  mAttributeName(aAttrName), mIsToken(true), mPermanentState(0),
  mValue1(aValue1), mState1(aState1),
  mValue2(aValue2), mState2(aState2),
  mValue3(aValue3), mState3(aState3),
  mDefaultState(0), mDefinedIfAbsent(true), mExcludingState(0)
{
  if (aDefaultStateRule == eUseFirstState)
    mDefaultState = aState1;
}

bool
nsStateMapEntry::MapToStates(nsIContent* aContent, PRUint64* aState,
                             eStateMapEntryID aStateMapEntryID)
{
  
  if (aStateMapEntryID == eARIANone)
    return false;

  const nsStateMapEntry& entry = nsARIAMap::gWAIStateMap[aStateMapEntryID];

  
  
  if (!entry.mAttributeName) {
    if (!(*aState & entry.mExcludingState))
      *aState |= entry.mDefaultState;

    return true;
  }

  if (entry.mIsToken) {
    
    
    bool hasAttr = aContent->HasAttr(kNameSpaceID_None, *entry.mAttributeName);
    if (entry.mDefinedIfAbsent && !hasAttr) {
      if (entry.mPermanentState)
        *aState |= entry.mPermanentState;
      if (entry.mState1)
        *aState |= entry.mState1;
      return true;
    }

    
    
    
    
    
    
    
    if (!hasAttr ||
        aContent->AttrValueIs(kNameSpaceID_None, *entry.mAttributeName,
                              nsGkAtoms::_empty, eCaseMatters) ||
        aContent->AttrValueIs(kNameSpaceID_None, *entry.mAttributeName,
                              nsGkAtoms::_undefined, eCaseMatters)) {

      if (entry.mPermanentState)
        *aState &= ~entry.mPermanentState;
      return true;
    }

    if (entry.mPermanentState)
      *aState |= entry.mPermanentState;
  }

  nsAutoString attrValue;
  if (!aContent->GetAttr(kNameSpaceID_None, *entry.mAttributeName, attrValue))
    return true;

  
  
  bool applyDefaultStates = true;
  if (entry.mValue1) {
    if (attrValue.EqualsASCII(entry.mValue1)) {
      applyDefaultStates = false;

      if (entry.mState1)
        *aState |= entry.mState1;
    } else if (entry.mValue2) {
      if (attrValue.EqualsASCII(entry.mValue2)) {
        applyDefaultStates = false;

        if (entry.mState2)
          *aState |= entry.mState2;

      } else if (entry.mValue3) {
        if (attrValue.EqualsASCII(entry.mValue3)) {
          applyDefaultStates = false;

          if (entry.mState3)
            *aState |= entry.mState3;

        }
      }
    }
  }

  if (applyDefaultStates) {
    if (entry.mDefaultState)
      *aState |= entry.mDefaultState;
  }

  return true;
}
