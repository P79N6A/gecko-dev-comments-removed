






































#include "nsARIAMap.h"

#include "Role.h"
#include "States.h"

#include "nsIContent.h"

using namespace mozilla::a11y;
using namespace mozilla::a11y::aria;
















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
    "form",
    roles::FORM,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
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
    "note",
    roles::NOTE,
    kUseMapRole,
    eNoValue,
    eNoAction,
    eNoLiveAttr,
    kNoReqStates
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






EStateRule nsARIAMap::gWAIUnivStateMap[] = {
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
