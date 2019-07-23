






































#include "nsARIAMap.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"














 

static const nsStateMapEntry kEndEntry = {nsnull, 0, 0};  

nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {
    "alert",
    nsIAccessibleRole::ROLE_ALERT,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "alertdialog",
    nsIAccessibleRole::ROLE_DIALOG,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "application",
    nsIAccessibleRole::ROLE_APPLICATION,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "article",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_READONLY,
    kEndEntry
  },
  {
    "button",
    nsIAccessibleRole::ROLE_PUSHBUTTON,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_pressed, kBoolState, nsIAccessibleStates::STATE_PRESSED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_pressed, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "checkbox",
    nsIAccessibleRole::ROLE_CHECKBUTTON,
    eNoValue,
    eCheckUncheckAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "columnheader",
    nsIAccessibleRole::ROLE_COLUMNHEADER,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "combobox",
    nsIAccessibleRole::ROLE_COMBOBOX,
    eHasValueMinMax,
    eOpenCloseAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_COLLAPSED | nsIAccessibleStates::STATE_HASPOPUP,
    
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "dialog",
    nsIAccessibleRole::ROLE_DIALOG,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "document",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_READONLY,
    kEndEntry
  },
  {
    "grid",
    nsIAccessibleRole::ROLE_TABLE,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_FOCUSABLE,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "gridcell",
    nsIAccessibleRole::ROLE_GRID_CELL,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "group",
    nsIAccessibleRole::ROLE_GROUPING,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "heading",
    nsIAccessibleRole::ROLE_HEADING,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "img",
    nsIAccessibleRole::ROLE_GRAPHIC,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "label",
    nsIAccessibleRole::ROLE_LABEL,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "link",
    nsIAccessibleRole::ROLE_LINK,
    eNoValue,
    eJumpAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_LINKED,
    kEndEntry
  },
  {
    "list",
    nsIAccessibleRole::ROLE_LIST,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listbox",
    nsIAccessibleRole::ROLE_LISTBOX,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listitem",
    nsIAccessibleRole::ROLE_LISTITEM,
    eNoValue,
    eNoAction, 
    eNoLiveAttr,
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "log",
    nsIAccessibleRole::ROLE_NOTHING,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "marquee",
    nsIAccessibleRole::ROLE_NOTHING,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "math",
    nsIAccessibleRole::ROLE_FLAT_EQUATION,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "menu",
    nsIAccessibleRole::ROLE_MENUPOPUP,
    eNoValue,
    eNoAction, 
               
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "menubar",
    nsIAccessibleRole::ROLE_MENUBAR,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "menuitem",
    nsIAccessibleRole::ROLE_MENUITEM,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "menuitemcheckbox",
    nsIAccessibleRole::ROLE_CHECK_MENU_ITEM,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    kEndEntry
  },
  {
    "menuitemradio",
    nsIAccessibleRole::ROLE_RADIO_MENU_ITEM,
    eNoValue,
    eClickAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    kEndEntry
  },
  {
    "option",
    nsIAccessibleRole::ROLE_OPTION,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "presentation",
    nsIAccessibleRole::ROLE_NOTHING,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "progressbar",
    nsIAccessibleRole::ROLE_PROGRESSBAR,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_READONLY,
    kEndEntry
  },
  {
    "radio",
    nsIAccessibleRole::ROLE_RADIOBUTTON,
    eNoValue,
    eSelectAction,
    eNoLiveAttr,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    kEndEntry
  },
  {
    "radiogroup",
    nsIAccessibleRole::ROLE_GROUPING,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "region",
    nsIAccessibleRole::ROLE_PANE,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "row",
    nsIAccessibleRole::ROLE_ROW,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    kEndEntry
  },
  {
    "rowheader",
    nsIAccessibleRole::ROLE_ROWHEADER,
    eNoValue,
    eSortAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "section",
    nsIAccessibleRole::ROLE_SECTION,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "separator",
    nsIAccessibleRole::ROLE_SEPARATOR,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "slider",
    nsIAccessibleRole::ROLE_SLIDER,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "spinbutton",
    nsIAccessibleRole::ROLE_SPINBUTTON,
    eHasValueMinMax,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "status",
    nsIAccessibleRole::ROLE_STATUSBAR,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "tab",
    nsIAccessibleRole::ROLE_PAGETAB,
    eNoValue,
    eSwitchAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "tablist",
    nsIAccessibleRole::ROLE_PAGETABLIST,
    eNoValue,
    eNoAction,
    ePoliteLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "tabpanel",
    nsIAccessibleRole::ROLE_PROPERTYPAGE,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "textbox",
    nsIAccessibleRole::ROLE_ENTRY,
    eNoValue,
    eActivateAction,
    eNoLiveAttr,
    kNoReqStates,
    
    
    {&nsAccessibilityAtoms::aria_autocomplete, "list", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_autocomplete, "both", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "timer",
    nsIAccessibleRole::ROLE_NOTHING,
    eNoValue,
    eNoAction,
    eOffLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "toolbar",
    nsIAccessibleRole::ROLE_TOOLBAR,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "tooltip",
    nsIAccessibleRole::ROLE_TOOLTIP,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    kEndEntry
  },
  {
    "tree",
    nsIAccessibleRole::ROLE_OUTLINE,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treegrid",
    nsIAccessibleRole::ROLE_TREE_TABLE,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treeitem",
    nsIAccessibleRole::ROLE_OUTLINEITEM,
    eNoValue,
    eActivateAction, 
                     
    eNoLiveAttr,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  }
};

PRUint32 nsARIAMap::gWAIRoleMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIRoleMap);

nsRoleMapEntry nsARIAMap::gLandmarkRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kNoReqStates,
  kEndEntry
};

nsRoleMapEntry nsARIAMap::gEmptyRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNoValue,
  eNoAction,
  eNoLiveAttr,
  kNoReqStates,
  kEndEntry
};






nsStateMapEntry nsARIAMap::gWAIUnivStateMap[] = {
  {&nsAccessibilityAtoms::aria_required, kBoolState, nsIAccessibleStates::STATE_REQUIRED},
  {&nsAccessibilityAtoms::aria_invalid,  kBoolState, nsIAccessibleStates::STATE_INVALID},
  {&nsAccessibilityAtoms::aria_haspopup, kBoolState, nsIAccessibleStates::STATE_HASPOPUP},
  {&nsAccessibilityAtoms::aria_busy,     "true",     nsIAccessibleStates::STATE_BUSY},
  {&nsAccessibilityAtoms::aria_busy,     "error",    nsIAccessibleStates::STATE_INVALID},
  {&nsAccessibilityAtoms::aria_disabled, kBoolState, nsIAccessibleStates::STATE_UNAVAILABLE},
  {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
  {&nsAccessibilityAtoms::aria_expanded, "false", nsIAccessibleStates::STATE_COLLAPSED},
  kEndEntry
};







nsAttributeCharacteristics nsARIAMap::gWAIUnivAttrMap[] = {
  {&nsAccessibilityAtoms::aria_activedescendant,  ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_atomic,                             ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_busy,                               ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_checked,           ATTR_BYPASSOBJ | ATTR_VALTOKEN }, 
  {&nsAccessibilityAtoms::aria_controls,          ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_describedby,       ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_disabled,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_dropeffect,                         ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_expanded,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_flowto,            ATTR_BYPASSOBJ                 },  
  {&nsAccessibilityAtoms::aria_grabbed,                            ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_haspopup,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_invalid,           ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_labelledby,        ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_live,                               ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_multiline,         ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_multiselectable,   ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_owns,              ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_pressed,           ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_readonly,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_relevant,          ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_required,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_selected,          ATTR_BYPASSOBJ | ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_sort,                               ATTR_VALTOKEN },
  {&nsAccessibilityAtoms::aria_valuenow,          ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_valuemin,          ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_valuemax,          ATTR_BYPASSOBJ                 },
  {&nsAccessibilityAtoms::aria_valuetext,         ATTR_BYPASSOBJ                 }
};

PRUint32 nsARIAMap::gWAIUnivAttrMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIUnivAttrMap);
