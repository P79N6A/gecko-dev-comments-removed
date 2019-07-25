






































#include "nsARIAMap.h"

#include "nsIAccessibleRole.h"
#include "States.h"

#include "nsIContent.h"

using namespace mozilla::a11y;
















nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {
    "alert",
    nsIAccessibleRole::ROLE_ALERT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "alertdialog",
    nsIAccessibleRole::ROLE_DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "application",
    nsIAccessibleRole::ROLE_APPLICATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "article",
    nsIAccessibleRole::ROLE_DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    states::READONLY
  },
  {
    "button",
    nsIAccessibleRole::ROLE_PUSHBUTTON,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAPressed
  },
  {
    "checkbox",
    nsIAccessibleRole::ROLE_CHECKBUTTON,
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
    nsIAccessibleRole::ROLE_COLUMNHEADER,
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
    nsIAccessibleRole::ROLE_COMBOBOX,
    kUseMapRole,
    eHasValueMinMax,
    eOpenCloseAction,
    eNoLiveAttr,
    states::COLLAPSED | states::HASPOPUP,
    eARIAAutoComplete,
    eARIAReadonly
  },
  {
    "dialog",
    nsIAccessibleRole::ROLE_DIALOG,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "directory",
    nsIAccessibleRole::ROLE_LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "document",
    nsIAccessibleRole::ROLE_DOCUMENT,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    states::READONLY
  },
  {
    "grid",
    nsIAccessibleRole::ROLE_TABLE,
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
    nsIAccessibleRole::ROLE_GRID_CELL,
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
    nsIAccessibleRole::ROLE_GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "heading",
    nsIAccessibleRole::ROLE_HEADING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "img",
    nsIAccessibleRole::ROLE_GRAPHIC,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "label",
    nsIAccessibleRole::ROLE_LABEL,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "link",
    nsIAccessibleRole::ROLE_LINK,
    kUseMapRole,
    eNoValue,
    eJumpAction,
    eNoLiveAttr,
    states::LINKED
  },
  {
    "list",
    nsIAccessibleRole::ROLE_LIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    states::READONLY,
    eARIAMultiSelectable
  },
  {
    "listbox",
    nsIAccessibleRole::ROLE_LISTBOX,
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
    nsIAccessibleRole::ROLE_LISTITEM,
    kUseMapRole,
    eNoValue,
    eNoAction, 
    eNoLiveAttr,
    states::READONLY,
    eARIASelectable,
    eARIACheckedMixed
  },
  {
    "log",
    nsIAccessibleRole::ROLE_NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "marquee",
    nsIAccessibleRole::ROLE_ANIMATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates
  },
  {
    "math",
    nsIAccessibleRole::ROLE_FLAT_EQUATION,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menu",
    nsIAccessibleRole::ROLE_MENUPOPUP,
    kUseMapRole,
    eNoValue,
    eNoAction, 
               
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menubar",
    nsIAccessibleRole::ROLE_MENUBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "menuitem",
    nsIAccessibleRole::ROLE_MENUITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckedMixed
  },
  {
    "menuitemcheckbox",
    nsIAccessibleRole::ROLE_CHECK_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableMixed
  },
  {
    "menuitemradio",
    nsIAccessibleRole::ROLE_RADIO_MENU_ITEM,
    kUseMapRole,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableBool
  },
  {
    "option",
    nsIAccessibleRole::ROLE_OPTION,
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
    nsIAccessibleRole::ROLE_NOTHING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "progressbar",
    nsIAccessibleRole::ROLE_PROGRESSBAR,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    states::READONLY
  },
  {
    "radio",
    nsIAccessibleRole::ROLE_RADIOBUTTON,
    kUseMapRole,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIACheckableBool
  },
  {
    "radiogroup",
    nsIAccessibleRole::ROLE_GROUPING,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "region",
    nsIAccessibleRole::ROLE_PANE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "row",
    nsIAccessibleRole::ROLE_ROW,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable
  },
  {
    "rowheader",
    nsIAccessibleRole::ROLE_ROWHEADER,
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
    nsIAccessibleRole::ROLE_SCROLLBAR,
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
    nsIAccessibleRole::ROLE_SEPARATOR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "slider",
    nsIAccessibleRole::ROLE_SLIDER,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAReadonly
  },
  {
    "spinbutton",
    nsIAccessibleRole::ROLE_SPINBUTTON,
    kUseMapRole,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIAReadonly
  },
  {
    "status",
    nsIAccessibleRole::ROLE_STATUSBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "tab",
    nsIAccessibleRole::ROLE_PAGETAB,
    kUseMapRole,
    eNoValue,
    eSwitchAction,
    eNoLiveAttr,
    kNoReqStates,
    eARIASelectable
  },
  {
    "tablist",
    nsIAccessibleRole::ROLE_PAGETABLIST,
    kUseMapRole,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates
  },
  {
    "tabpanel",
    nsIAccessibleRole::ROLE_PROPERTYPAGE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "textbox",
    nsIAccessibleRole::ROLE_ENTRY,
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
    nsIAccessibleRole::ROLE_NOTHING,
    kUseNativeRole,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates
  },
  {
    "toolbar",
    nsIAccessibleRole::ROLE_TOOLBAR,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "tooltip",
    nsIAccessibleRole::ROLE_TOOLTIP,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
  },
  {
    "tree",
    nsIAccessibleRole::ROLE_OUTLINE,
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
    nsIAccessibleRole::ROLE_TREE_TABLE,
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
    nsIAccessibleRole::ROLE_OUTLINEITEM,
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
  nsIAccessibleRole::ROLE_NOTHING,
  kUseNativeRole,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kNoReqStates
};

nsRoleMapEntry nsARIAMap::gEmptyRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
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
                  states::CHECKABLE, states::CHECKED, 0, PR_TRUE),

  
  nsStateMapEntry(&nsGkAtoms::aria_checked, kMixedType,
                  states::CHECKABLE, states::CHECKED, 0, PR_TRUE),

  
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
                  0, states::MULTI_LINE, states::SINGLE_LINE, PR_TRUE),

  
  nsStateMapEntry(&nsGkAtoms::aria_multiselectable, kBoolType,
                  0, states::MULTISELECTABLE | states::EXTSELECTABLE),

  
  nsStateMapEntry(&nsGkAtoms::aria_orientation, eUseFirstState,
                  "vertical", states::VERTICAL,
                  "horizontal", states::HORIZONTAL),

  
  nsStateMapEntry(&nsGkAtoms::aria_pressed, kMixedType,
                  states::CHECKABLE, states::PRESSED),

  
  nsStateMapEntry(&nsGkAtoms::aria_readonly, kBoolType,
                  0, states::READONLY),

  
  nsStateMapEntry(&nsGkAtoms::aria_readonly, kBoolType,
                  0, states::READONLY, states::EDITABLE, PR_TRUE),

  
  nsStateMapEntry(&nsGkAtoms::aria_required, kBoolType,
                  0, states::REQUIRED),

  
  nsStateMapEntry(&nsGkAtoms::aria_selected, kBoolType,
                  states::SELECTABLE, states::SELECTED, 0, PR_TRUE)
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
  mIsToken(PR_FALSE),
  mPermanentState(0),
  mValue1(nsnull),
  mState1(0),
  mValue2(nsnull),
  mState2(0),
  mValue3(nsnull),
  mState3(0),
  mDefaultState(0),
  mDefinedIfAbsent(PR_FALSE)
{}

nsStateMapEntry::nsStateMapEntry(nsIAtom** aAttrName, eStateValueType aType,
                                 PRUint64 aPermanentState,
                                 PRUint64 aTrueState,
                                 PRUint64 aFalseState,
                                 bool aDefinedIfAbsent) :
  mAttributeName(aAttrName),
  mIsToken(PR_TRUE),
  mPermanentState(aPermanentState),
  mValue1("false"),
  mState1(aFalseState),
  mValue2(nsnull),
  mState2(0),
  mValue3(nsnull),
  mState3(0),
  mDefaultState(aTrueState),
  mDefinedIfAbsent(aDefinedIfAbsent)
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
  mAttributeName(aAttrName), mIsToken(PR_FALSE), mPermanentState(0),
  mValue1(aValue1), mState1(aState1),
  mValue2(aValue2), mState2(aState2),
  mValue3(aValue3), mState3(aState3),
  mDefaultState(0), mDefinedIfAbsent(PR_FALSE)
{
}

nsStateMapEntry::nsStateMapEntry(nsIAtom** aAttrName,
                                 EDefaultStateRule aDefaultStateRule,
                                 const char* aValue1, PRUint64 aState1,
                                 const char* aValue2, PRUint64 aState2,
                                 const char* aValue3, PRUint64 aState3) :
  mAttributeName(aAttrName), mIsToken(PR_TRUE), mPermanentState(0),
  mValue1(aValue1), mState1(aState1),
  mValue2(aValue2), mState2(aState2),
  mValue3(aValue3), mState3(aState3),
  mDefaultState(0), mDefinedIfAbsent(PR_TRUE)
{
  if (aDefaultStateRule == eUseFirstState)
    mDefaultState = aState1;
}

bool
nsStateMapEntry::MapToStates(nsIContent* aContent, PRUint64* aState,
                             eStateMapEntryID aStateMapEntryID)
{
  
  if (aStateMapEntryID == eARIANone)
    return PR_FALSE;

  const nsStateMapEntry& entry = nsARIAMap::gWAIStateMap[aStateMapEntryID];

  if (entry.mIsToken) {
    
    
    bool hasAttr = aContent->HasAttr(kNameSpaceID_None, *entry.mAttributeName);
    if (entry.mDefinedIfAbsent && !hasAttr) {
      if (entry.mPermanentState)
        *aState |= entry.mPermanentState;
      if (entry.mState1)
        *aState |= entry.mState1;
      return PR_TRUE;
    }

    
    
    
    
    
    
    
    if (!hasAttr ||
        aContent->AttrValueIs(kNameSpaceID_None, *entry.mAttributeName,
                              nsGkAtoms::_empty, eCaseMatters) ||
        aContent->AttrValueIs(kNameSpaceID_None, *entry.mAttributeName,
                              nsGkAtoms::_undefined, eCaseMatters)) {

      if (entry.mPermanentState)
        *aState &= ~entry.mPermanentState;
      return PR_TRUE;
    }

    if (entry.mPermanentState)
      *aState |= entry.mPermanentState;
  }

  nsAutoString attrValue;
  if (!aContent->GetAttr(kNameSpaceID_None, *entry.mAttributeName, attrValue))
    return PR_TRUE;

  
  
  bool applyDefaultStates = true;
  if (entry.mValue1) {
    if (attrValue.EqualsASCII(entry.mValue1)) {
      applyDefaultStates = PR_FALSE;

      if (entry.mState1)
        *aState |= entry.mState1;
    } else if (entry.mValue2) {
      if (attrValue.EqualsASCII(entry.mValue2)) {
        applyDefaultStates = PR_FALSE;

        if (entry.mState2)
          *aState |= entry.mState2;

      } else if (entry.mValue3) {
        if (attrValue.EqualsASCII(entry.mValue3)) {
          applyDefaultStates = PR_FALSE;

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

  return PR_TRUE;
}
