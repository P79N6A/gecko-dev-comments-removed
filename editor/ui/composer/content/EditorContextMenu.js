






































function EditorFillContextMenu(event, contextMenuNode)
{
  if ( event.target != contextMenuNode )
    return;

  
  var objectName = InitObjectPropertiesMenuitem("objectProperties_cm");
  var isInLink = objectName == "href";

  
  if (objectName == "img")
  try {
    isInLink = GetCurrentEditor().getElementOrParentByTagName("href", GetObjectForProperties());
  } catch (e) {}

  InitRemoveStylesMenuitems("removeStylesMenuitem_cm", "removeLinksMenuitem_cm", "removeNamedAnchorsMenuitem_cm");

  var inCell = IsInTableCell();
  
  InitJoinCellMenuitem("joinTableCells_cm");

  
  goUpdateTableMenuItems(document.getElementById("composerTableMenuItems"));

  
  var children = contextMenuNode.childNodes;
  if (children)
  {
    var count = children.length;
    for (var i = 0; i < count; i++)
      HideDisabledItem(children[i]);
  }

  
  
  ShowMenuItem("createLink_cm", !isInLink);

  
  ShowMenuItem("editLink_cm", isInLink);

  
  
  var haveUndo =
    IsMenuItemShowing("menu_undo_cm") ||
    IsMenuItemShowing("menu_redo_cm");

  var haveEdit =
    IsMenuItemShowing("menu_cut_cm")   ||
    IsMenuItemShowing("menu_copy_cm")  ||
    IsMenuItemShowing("menu_paste_cm") ||
    IsMenuItemShowing("menu_pasteNoFormatting_cm") ||
    IsMenuItemShowing("menu_delete_cm");

  var haveStyle =
    IsMenuItemShowing("removeStylesMenuitem_cm") ||
    IsMenuItemShowing("createLink_cm") ||
    IsMenuItemShowing("removeLinksMenuitem_cm") ||
    IsMenuItemShowing("removeNamedAnchorsMenuitem_cm");

  var haveProps =
    IsMenuItemShowing("objectProperties_cm");

  ShowMenuItem("undoredo-separator", haveUndo && haveEdit);

  ShowMenuItem("edit-separator", haveEdit || haveUndo);

  
  
  

  var showStyleSep = haveStyle && (haveProps || inCell);
  ShowMenuItem("styles-separator", showStyleSep);

  var showPropSep = (haveProps && inCell);
  ShowMenuItem("property-separator", showPropSep);

  
  ShowMenuItem("tableInsertMenu_cm",  inCell);
  ShowMenuItem("tableSelectMenu_cm",  inCell);
  ShowMenuItem("tableDeleteMenu_cm",  inCell);

  
  
  InlineSpellCheckerUI.clearSuggestionsFromMenu();
  InlineSpellCheckerUI.initFromEvent(document.popupRangeParent, document.popupRangeOffset);
  var onMisspelling = InlineSpellCheckerUI.overMisspelling;
  document.getElementById('spellCheckSuggestionsSeparator').hidden = !onMisspelling;
  document.getElementById('spellCheckAddToDictionary').hidden = !onMisspelling;
  document.getElementById('spellCheckIgnoreWord').hidden = !onMisspelling;
  var separator = document.getElementById('spellCheckAddSep');
  separator.hidden = !onMisspelling;
  document.getElementById('spellCheckNoSuggestions').hidden = !onMisspelling ||
      InlineSpellCheckerUI.addSuggestionsToMenu(contextMenuNode, separator, 5);
}

function IsItemOrCommandEnabled( item )
{
  var command = item.getAttribute("command");
  if (command) {
    
    var controller = document.commandDispatcher.getControllerForCommand(command);
    if (controller)
      return controller.isCommandEnabled(command);
  }

  
  return item.getAttribute("disabled") != "true";
}

function HideDisabledItem( item )
{
  item.hidden = !IsItemOrCommandEnabled(item);
}

function ShowMenuItem(id, showItem)
{
  var item = document.getElementById(id);
  if (item && !showItem)
  {
    item.hidden = true;
  }
  
}

function IsMenuItemShowing(menuID)
{
  var item = document.getElementById(menuID);
  if (item)
    return !item.hidden;

  return false;
}

