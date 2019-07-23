






































#include "nsARIAMap.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleStates.h"















 

static const nsStateMapEntry kEndEntry = {0, 0, 0};  

nsRoleMapEntry nsARIAMap::gWAIRoleMap[] = 
{
  {"alert", nsIAccessibleRole::ROLE_ALERT, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"alertdialog", nsIAccessibleRole::ROLE_ALERT, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"application", nsIAccessibleRole::ROLE_APPLICATION, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"button", nsIAccessibleRole::ROLE_PUSHBUTTON, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"pressed", kBoolState, nsIAccessibleStates::STATE_PRESSED}, kEndEntry},
  {"buttonsubmit", nsIAccessibleRole::ROLE_PUSHBUTTON, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_DEFAULT, kEndEntry},
  {"buttoncancel", nsIAccessibleRole::ROLE_PUSHBUTTON, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"checkbox", nsIAccessibleRole::ROLE_CHECKBUTTON, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_CHECKABLE,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"checkboxtristate", nsIAccessibleRole::ROLE_CHECKBUTTON, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_CHECKABLE,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED},
            {"checked", "mixed", nsIAccessibleStates::STATE_MIXED},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"columnheader", nsIAccessibleRole::ROLE_COLUMNHEADER, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"combobox", nsIAccessibleRole::ROLE_COMBOBOX, eNameLabelOrTitle, eHasValueMinMax, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY},
            {"expanded", kBoolState, nsIAccessibleStates::STATE_EXPANDED},
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE}, kEndEntry},
  {"description", nsIAccessibleRole::ROLE_TEXT_CONTAINER, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"dialog", nsIAccessibleRole::ROLE_DIALOG, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"document", nsIAccessibleRole::ROLE_DOCUMENT, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"grid", nsIAccessibleRole::ROLE_TABLE, eNameLabelOrTitle, eNoValue, nsIAccessibleStates::STATE_FOCUSABLE,
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"gridcell", nsIAccessibleRole::ROLE_CELL, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"expanded", kBoolState, nsIAccessibleStates::STATE_EXPANDED},
            {"expanded", "false", nsIAccessibleStates::STATE_COLLAPSED},
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"group", nsIAccessibleRole::ROLE_GROUPING, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"label", nsIAccessibleRole::ROLE_LABEL, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"link", nsIAccessibleRole::ROLE_LINK, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_LINKED, kEndEntry},
  {"list", nsIAccessibleRole::ROLE_LIST, eNameLabelOrTitle, eNoValue, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY},
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE}, kEndEntry},
  {"listbox", nsIAccessibleRole::ROLE_LIST, eNameLabelOrTitle, eNoValue, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY},
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE}, kEndEntry},
  {"listitem", nsIAccessibleRole::ROLE_LISTITEM, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
            {"checked", "false", nsIAccessibleStates::STATE_CHECKABLE}, kEndEntry},
  {"menu", nsIAccessibleRole::ROLE_MENUPOPUP, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"menubar", nsIAccessibleRole::ROLE_MENUBAR, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"menuitem", nsIAccessibleRole::ROLE_MENUITEM, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
            {"checked", "mixed", nsIAccessibleStates::STATE_MIXED},
            {"checked", "false", nsIAccessibleStates::STATE_CHECKABLE}, kEndEntry},
  {"menuitemcheckbox", nsIAccessibleRole::ROLE_MENUITEM, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_CHECKABLE,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED }, kEndEntry},
  {"menuitemradio", nsIAccessibleRole::ROLE_MENUITEM, eNameOkFromChildren, eNoValue, nsIAccessibleStates::STATE_CHECKABLE,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED }, kEndEntry},
  {"option", nsIAccessibleRole::ROLE_LISTITEM, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
            {"checked", "false", nsIAccessibleStates::STATE_CHECKABLE}, kEndEntry},
  {"progressbar", nsIAccessibleRole::ROLE_PROGRESSBAR, eNameLabelOrTitle, eHasValueMinMax, nsIAccessibleStates::STATE_READONLY, kEndEntry},
  {"radio", nsIAccessibleRole::ROLE_RADIOBUTTON, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED}, kEndEntry},
  {"radiogroup", nsIAccessibleRole::ROLE_GROUPING, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"row", nsIAccessibleRole::ROLE_ROW, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"expanded", kBoolState, nsIAccessibleStates::STATE_EXPANDED},
            {"expanded", "false", nsIAccessibleStates::STATE_COLLAPSED}, kEndEntry},
  {"rowheader", nsIAccessibleRole::ROLE_ROWHEADER, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"secret", nsIAccessibleRole::ROLE_PASSWORD_TEXT, eNameLabelOrTitle, eNoValue, nsIAccessibleStates::STATE_PROTECTED,
             kEndEntry},  
  {"separator", nsIAccessibleRole::ROLE_SEPARATOR, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"slider", nsIAccessibleRole::ROLE_SLIDER, eNameLabelOrTitle, eHasValueMinMax, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"spinbutton", nsIAccessibleRole::ROLE_SPINBUTTON, eNameLabelOrTitle, eHasValueMinMax, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry},
  {"spreadsheet", nsIAccessibleRole::ROLE_TABLE, eNameLabelOrTitle, eNoValue, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE | nsIAccessibleStates::STATE_FOCUSABLE,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry}, 
  {"status", nsIAccessibleRole::ROLE_STATUSBAR, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"tab", nsIAccessibleRole::ROLE_PAGETAB, eNameOkFromChildren, eNoValue, kNoReqStates, kEndEntry},
  {"tablist", nsIAccessibleRole::ROLE_PAGETABLIST, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"tabpanel", nsIAccessibleRole::ROLE_PROPERTYPAGE, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"textarea", nsIAccessibleRole::ROLE_ENTRY, eNameLabelOrTitle, eHasValueMinMax, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry}, 
  {"textfield", nsIAccessibleRole::ROLE_ENTRY, eNameLabelOrTitle, eHasValueMinMax, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY}, kEndEntry}, 
  {"toolbar", nsIAccessibleRole::ROLE_TOOLBAR, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {"tree", nsIAccessibleRole::ROLE_OUTLINE, eNameLabelOrTitle, eNoValue, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY},
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE}, kEndEntry},
  {"treegrid", nsIAccessibleRole::ROLE_TREE_TABLE, eNameLabelOrTitle, eNoValue, kNoReqStates,
            {"readonly", kBoolState, nsIAccessibleStates::STATE_READONLY},
            {"multiselectable", kBoolState, nsIAccessibleStates::STATE_MULTISELECTABLE | nsIAccessibleStates::STATE_EXTSELECTABLE}, kEndEntry},
  {"treeitem", nsIAccessibleRole::ROLE_OUTLINEITEM, eNameOkFromChildren, eNoValue, kNoReqStates,
            {"selected", kBoolState, nsIAccessibleStates::STATE_SELECTED | nsIAccessibleStates::STATE_SELECTABLE},
            {"selected", "false", nsIAccessibleStates::STATE_SELECTABLE},
            {"expanded", kBoolState, nsIAccessibleStates::STATE_EXPANDED},
            {"expanded", "false", nsIAccessibleStates::STATE_COLLAPSED},
            {"checked", kBoolState, nsIAccessibleStates::STATE_CHECKED | nsIAccessibleStates::STATE_CHECKABLE},
            {"checked", "mixed", nsIAccessibleStates::STATE_MIXED},
            {"checked", "false", nsIAccessibleStates::STATE_CHECKABLE},},
  {"treegroup", nsIAccessibleRole::ROLE_GROUPING, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry},
  {nsnull, nsIAccessibleRole::ROLE_NOTHING, eNameLabelOrTitle, eNoValue, kNoReqStates, kEndEntry} 
};






nsStateMapEntry nsARIAMap::gWAIUnivStateMap[] = {
  {"disabled", kBoolState, nsIAccessibleStates::STATE_UNAVAILABLE},
  {"required", kBoolState, nsIAccessibleStates::STATE_REQUIRED},
  {"invalid",  kBoolState, nsIAccessibleStates::STATE_INVALID},
  {"haspopup", kBoolState, nsIAccessibleStates::STATE_HASPOPUP},
  {"busy",     "true",     nsIAccessibleStates::STATE_BUSY},
  {"busy",     "error",    nsIAccessibleStates::STATE_INVALID},
  kEndEntry
};

