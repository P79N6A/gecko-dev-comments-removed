






































#include "nsARIAMap.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"














 

static const nsStateMapEntry kEndEntry = {nsnull, 0, 0};  

nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {
    "alert",
    nsIAccessibleRole::ROLE_ALERT,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "alertdialog",
    nsIAccessibleRole::ROLE_ALERT,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "application",
    nsIAccessibleRole::ROLE_APPLICATION,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "article",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "button",
    nsIAccessibleRole::ROLE_PUSHBUTTON,
    eNameOkFromChildren,
    eNoValue,
    eClickAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_pressed, kBoolState, nsIAccessibleStates::STATE_PRESSED},
    {&nsAccessibilityAtoms::aria_pressed, "mixed", nsIAccessibleStates::STATE_MIXED},
    kEndEntry
  },
  {
    "checkbox",
    nsIAccessibleRole::ROLE_CHECKBUTTON,
    eNameOkFromChildren,
    eNoValue,
    eCheckUncheckAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "columnheader",
    nsIAccessibleRole::ROLE_COLUMNHEADER,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "combobox",
    nsIAccessibleRole::ROLE_COMBOBOX,
    eNameLabelOrTitle,
    eHasValueMinMax,
    eOpenCloseAction,
    nsIAccessibleStates::STATE_COLLAPSED | nsIAccessibleStates::STATE_HASPOPUP,
    
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
    kEndEntry
  },
  {
    "description",
    nsIAccessibleRole::ROLE_TEXT_CONTAINER,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "dialog",
    nsIAccessibleRole::ROLE_DIALOG,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "document",
    nsIAccessibleRole::ROLE_DOCUMENT,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "grid",
    nsIAccessibleRole::ROLE_TABLE,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    nsIAccessibleStates::STATE_FOCUSABLE,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "gridcell",
    nsIAccessibleRole::ROLE_CELL,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
    {&nsAccessibilityAtoms::aria_expanded, "false", nsIAccessibleStates::STATE_COLLAPSED},
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "group",
    nsIAccessibleRole::ROLE_GROUPING,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "heading",
    nsIAccessibleRole::ROLE_HEADING,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "img",
    nsIAccessibleRole::ROLE_GRAPHIC,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "label",
    nsIAccessibleRole::ROLE_LABEL,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "link",
    nsIAccessibleRole::ROLE_LINK,
    eNameOkFromChildren,
    eNoValue,
    eJumpAction,
    nsIAccessibleStates::STATE_LINKED,
    kEndEntry
  },
  {
    "list",
    nsIAccessibleRole::ROLE_LIST,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listbox",
    nsIAccessibleRole::ROLE_LISTBOX,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "listitem",
    nsIAccessibleRole::ROLE_LISTITEM,
    eNameOkFromChildren,
    eNoValue,
    eNoAction, 
    nsIAccessibleStates::STATE_READONLY,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "math",
    nsIAccessibleRole::ROLE_FLAT_EQUATION,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "menu",
    nsIAccessibleRole::ROLE_MENUPOPUP,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction, 
               
    kNoReqStates,
    kEndEntry
  },
  {
    "menubar",
    nsIAccessibleRole::ROLE_MENUBAR,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "menuitem",
    nsIAccessibleRole::ROLE_MENUITEM,
    eNameOkFromChildren,
    eNoValue,
    eClickAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
    kEndEntry
  },
  {
    "menuitemcheckbox",
    nsIAccessibleRole::ROLE_CHECK_MENU_ITEM,
    eNameOkFromChildren,
    eNoValue,
    eClickAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED},
    kEndEntry
  },
  {
    "menuitemradio",
    nsIAccessibleRole::ROLE_RADIO_MENU_ITEM,
    eNameOkFromChildren,
    eNoValue,
    eClickAction,
    nsIAccessibleStates::STATE_CHECKABLE,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED },
    kEndEntry
  },
  {
    "option",
    nsIAccessibleRole::ROLE_OPTION,
    eNameOkFromChildren,
    eNoValue,
    eSelectAction,
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
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "progressbar",
    nsIAccessibleRole::ROLE_PROGRESSBAR,
    eNameLabelOrTitle,
    eHasValueMinMax,
    eNoAction,
    nsIAccessibleStates::STATE_READONLY,
    kEndEntry
  },
  {
    "radio",
    nsIAccessibleRole::ROLE_RADIOBUTTON,
    eNameOkFromChildren,
    eNoValue,
    eSelectAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED},
    kEndEntry
  },
  {
    "radiogroup",
    nsIAccessibleRole::ROLE_GROUPING,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "region",
    nsIAccessibleRole::ROLE_PANE,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "row",
    nsIAccessibleRole::ROLE_ROW,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
    {&nsAccessibilityAtoms::aria_expanded, "false", nsIAccessibleStates::STATE_COLLAPSED},
    kEndEntry
  },
  {
    "rowheader",
    nsIAccessibleRole::ROLE_ROWHEADER,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "section",
    nsIAccessibleRole::ROLE_SECTION,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "separator",
    nsIAccessibleRole::ROLE_SEPARATOR,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "slider",
    nsIAccessibleRole::ROLE_SLIDER,
    eNameLabelOrTitle,
    eHasValueMinMax,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "spinbutton",
    nsIAccessibleRole::ROLE_SPINBUTTON,
    eNameLabelOrTitle,
    eHasValueMinMax,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "status",
    nsIAccessibleRole::ROLE_STATUSBAR,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tab",
    nsIAccessibleRole::ROLE_PAGETAB,
    eNameOkFromChildren,
    eNoValue,
    eSwitchAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tablist",
    nsIAccessibleRole::ROLE_PAGETABLIST,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tabpanel",
    nsIAccessibleRole::ROLE_PROPERTYPAGE,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "textbox",
    nsIAccessibleRole::ROLE_ENTRY,
    eNameLabelOrTitle,
    eNoValue,
    eActivateAction,
    kNoReqStates,
    
    
    {&nsAccessibilityAtoms::aria_autocomplete, "list", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_autocomplete, "both", nsIAccessibleStates::STATE_HASPOPUP},
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    kEndEntry
  },
  {
    "toolbar",
    nsIAccessibleRole::ROLE_TOOLBAR,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tooltip",
    nsIAccessibleRole::ROLE_TOOLTIP,
    eNameOkFromChildren,
    eNoValue,
    eNoAction,
    kNoReqStates,
    kEndEntry
  },
  {
    "tree",
    nsIAccessibleRole::ROLE_OUTLINE,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treegrid",
    nsIAccessibleRole::ROLE_TREE_TABLE,
    eNameLabelOrTitle,
    eNoValue,
    eNoAction,
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_readonly, kBoolState, nsIAccessibleStates::STATE_READONLY},
    {&nsAccessibilityAtoms::aria_multiselectable, kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
    kEndEntry
  },
  {
    "treeitem",
    nsIAccessibleRole::ROLE_OUTLINEITEM,
    eNameOkFromChildren,
    eNoValue,
    eActivateAction, 
                     
    kNoReqStates,
    {&nsAccessibilityAtoms::aria_selected, kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_selected, "false", nsIAccessibleStates::STATE_SELECTABLE},
    {&nsAccessibilityAtoms::aria_expanded, kBoolState, nsIAccessibleStates::STATE_EXPANDED},
    {&nsAccessibilityAtoms::aria_expanded, "false", nsIAccessibleStates::STATE_COLLAPSED},
    {&nsAccessibilityAtoms::aria_checked, kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "mixed", nsIAccessibleStates::STATE_MIXED | nsIAccessibleStates::STATE_CHECKABLE},
    {&nsAccessibilityAtoms::aria_checked, "false", nsIAccessibleStates::STATE_CHECKABLE},
  },
};

PRUint32 nsARIAMap::gWAIRoleMapLength = NS_ARRAY_LENGTH(nsARIAMap::gWAIRoleMap);

nsRoleMapEntry nsARIAMap::gLandmarkRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNameLabelOrTitle,
  eNoValue,
  eNoAction,
  kNoReqStates,
  kEndEntry
};

nsRoleMapEntry nsARIAMap::gEmptyRoleMap = {
  "",
  nsIAccessibleRole::ROLE_NOTHING,
  eNameLabelOrTitle,
  eNoValue,
  eNoAction,
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
  kEndEntry
};

