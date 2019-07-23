# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is
# David Hyatt.
# Portions created by the Initial Developer are Copyright (C) 2002
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   David Hyatt (hyatt@apple.com)
#   Blake Ross (blaker@netscape.com)
#   Joe Hewitt (hewitt@netscape.com)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const kRowMax = 4;

var gToolboxDocument = null;
var gToolbox = null;
var gCurrentDragOverItem = null;
var gToolboxChanged = false;
var gToolboxIconSize = false;

function onLoad()
{
  InitWithToolbox(window.arguments[0]);
  repositionDialog();
}

function InitWithToolbox(aToolbox)
{
  gToolbox = aToolbox;
  gToolboxDocument = gToolbox.ownerDocument;
  
  gToolbox.addEventListener("draggesture", onToolbarDragGesture, false);
  gToolbox.addEventListener("dragover", onToolbarDragOver, false);
  gToolbox.addEventListener("dragexit", onToolbarDragExit, false);
  gToolbox.addEventListener("dragdrop", onToolbarDragDrop, false);

  initDialog();
}

function finishToolbarCustomization()
{
  removeToolboxListeners();
  unwrapToolbarItems();
  persistCurrentSets();
  
  notifyParentComplete();
}

function initDialog()
{
  var mode = gToolbox.getAttribute("mode");
  document.getElementById("modelist").value = mode;
  gToolboxIconSize = gToolbox.getAttribute("iconsize");
  var smallIconsCheckbox = document.getElementById("smallicons");
  smallIconsCheckbox.checked = gToolboxIconSize == "small";
  if (mode == "text")
    smallIconsCheckbox.disabled = true;

  
  buildPalette();

  
  wrapToolbarItems();
}

function repositionDialog()
{
  
  
  var width;
  if (document.documentElement.hasAttribute("width"))
    width = document.documentElement.getAttribute("width");
  else
    width = parseInt(document.documentElement.style.width);
  var screenX = gToolbox.boxObject.screenX 
                + ((gToolbox.boxObject.width - width) / 2);
  var screenY = gToolbox.boxObject.screenY + gToolbox.boxObject.height;

  window.moveTo(screenX, screenY);
}

function removeToolboxListeners()
{
  gToolbox.removeEventListener("draggesture", onToolbarDragGesture, false);
  gToolbox.removeEventListener("dragover", onToolbarDragOver, false);
  gToolbox.removeEventListener("dragexit", onToolbarDragExit, false);
  gToolbox.removeEventListener("dragdrop", onToolbarDragDrop, false);
}





function notifyParentComplete()
{
  if ("customizeDone" in gToolbox)
    gToolbox.customizeDone(gToolboxChanged);
}

function getToolbarAt(i)
{
  return gToolbox.childNodes[i];
}





function persistCurrentSets()
{
  if (!gToolboxChanged)
    return;

  var customCount = 0;
  for (var i = 0; i < gToolbox.childNodes.length; ++i) {
    
    var toolbar = getToolbarAt(i);
    if (isCustomizableToolbar(toolbar)) {
      
      var currentSet = toolbar.currentSet;
      toolbar.setAttribute("currentset", currentSet);
      
      var customIndex = toolbar.hasAttribute("customindex");
      if (customIndex) {
        if (!toolbar.firstChild) {
          
          gToolbox.removeChild(toolbar);
          --i;
        } else {
          
          gToolbox.toolbarset.setAttribute("toolbar"+(++customCount),
                                           toolbar.toolbarName + ":" + currentSet);
          gToolboxDocument.persist(gToolbox.toolbarset.id, "toolbar"+customCount);
        }
      }

      if (!customIndex) {
        
        gToolboxDocument.persist(toolbar.id, "currentset");
      }
    }
  }
  
  
  while (gToolbox.toolbarset.hasAttribute("toolbar"+(++customCount))) {
    gToolbox.toolbarset.removeAttribute("toolbar"+customCount);
    gToolboxDocument.persist(gToolbox.toolbarset.id, "toolbar"+customCount);
  }
}




function wrapToolbarItems()
{
  for (var i = 0; i < gToolbox.childNodes.length; ++i) {
    var toolbar = getToolbarAt(i);
    if (isCustomizableToolbar(toolbar)) {
      for (var k = 0; k < toolbar.childNodes.length; ++k) {
        var item = toolbar.childNodes[k];
        if (isToolbarItem(item)) {
          var nextSibling = item.nextSibling;
          
          var wrapper = wrapToolbarItem(item);
          
          if (nextSibling)
            toolbar.insertBefore(wrapper, nextSibling);
          else
            toolbar.appendChild(wrapper);

          cleanupItemForToolbar(item, wrapper);
        }
      }
    }
  }
}




function unwrapToolbarItems()
{
  var paletteItems = gToolbox.getElementsByTagName("toolbarpaletteitem");
  var paletteItem;
  while ((paletteItem = paletteItems.item(0)) != null) {
    var toolbarItem = paletteItem.firstChild;

    if (paletteItem.hasAttribute("itemdisabled"))
      toolbarItem.disabled = true;

    if (paletteItem.hasAttribute("itemcommand"))
      toolbarItem.setAttribute("command", paletteItem.getAttribute("itemcommand"));

    paletteItem.parentNode.replaceChild(toolbarItem, paletteItem);
  }
}





function createWrapper(aId)
{
  var wrapper = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                         "toolbarpaletteitem");

  wrapper.id = "wrapper-"+aId;  
  return wrapper;
}





function wrapPaletteItem(aPaletteItem, aCurrentRow, aSpacer)
{
  var wrapper = createWrapper(aPaletteItem.id);

  wrapper.setAttribute("flex", 1);
  wrapper.setAttribute("align", "center");
  wrapper.setAttribute("pack", "center");
  wrapper.setAttribute("minheight", "0");
  wrapper.setAttribute("minwidth", "0");

  document.adoptNode(aPaletteItem);
  wrapper.appendChild(aPaletteItem);
  
  
  
  
  cleanUpItemForPalette(aPaletteItem, wrapper);

  if (aSpacer)
    aCurrentRow.insertBefore(wrapper, aSpacer);
  else
    aCurrentRow.appendChild(wrapper);

}






function wrapToolbarItem(aToolbarItem)
{
  var wrapper = createWrapper(aToolbarItem.id);
  gToolboxDocument.adoptNode(wrapper);

  wrapper.flex = aToolbarItem.flex;

  if (aToolbarItem.parentNode)
    aToolbarItem.parentNode.removeChild(aToolbarItem);
  
  wrapper.appendChild(aToolbarItem);
  
  return wrapper;
}




function getCurrentItemIds()
{
  var currentItems = {};
  for (var i = 0; i < gToolbox.childNodes.length; ++i) {
    var toolbar = getToolbarAt(i);
    if (isCustomizableToolbar(toolbar)) {
      var child = toolbar.firstChild;
      while (child) {
        if (isToolbarItem(child))
          currentItems[child.id] = 1;
        child = child.nextSibling;
      }
    }
  }
  return currentItems;
}




function buildPalette()
{
  
  var paletteBox = document.getElementById("palette-box");
  while (paletteBox.lastChild)
    paletteBox.removeChild(paletteBox.lastChild);

  var currentRow = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                            "hbox");
  currentRow.setAttribute("class", "paletteRow");

  
  var templateNode = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                              "toolbarseparator");
  templateNode.id = "separator";
  wrapPaletteItem(templateNode, currentRow, null);

  
  templateNode = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                              "toolbarspring");
  templateNode.id = "spring";
  templateNode.flex = 1;
  wrapPaletteItem(templateNode, currentRow, null);

  
  templateNode = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                              "toolbarspacer");
  templateNode.id = "spacer";
  templateNode.flex = 1;
  wrapPaletteItem(templateNode, currentRow, null);

  
  templateNode = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                          "splitter");
  templateNode.id = "splitter";
  wrapPaletteItem(templateNode, currentRow, null);

  var rowSlot = 3;

  var currentItems = getCurrentItemIds();
  templateNode = gToolbox.palette.firstChild;
  while (templateNode) {
    
    if (!(templateNode.id in currentItems)) {
      var paletteItem = templateNode.cloneNode(true);

      if (rowSlot == kRowMax) {
        
        paletteBox.appendChild(currentRow);

        
        currentRow = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                              "hbox");
        currentRow.setAttribute("class", "paletteRow");
        rowSlot = 0;
      }

      ++rowSlot;
      wrapPaletteItem(paletteItem, currentRow, null);
    }
    
    templateNode = templateNode.nextSibling;
  }

  if (currentRow) { 
    fillRowWithFlex(currentRow);
    paletteBox.appendChild(currentRow);
  }
}





function appendPaletteItem(aItem)
{
  var paletteBox = document.getElementById("palette-box");
  var lastRow = paletteBox.lastChild;
  var lastSpacer = lastRow.lastChild;
   
  if (lastSpacer.localName != "spacer") {
    
    lastRow = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                        "hbox");
    lastRow.setAttribute("class", "paletteRow");
    paletteBox.appendChild(lastRow);
    
    wrapPaletteItem(aItem, lastRow, null);

    fillRowWithFlex(lastRow);
  } else {
    
    var flex = lastSpacer.getAttribute("flex");
    if (flex == 1) {
      lastRow.removeChild(lastSpacer);
      lastSpacer = null;
    } else
      lastSpacer.setAttribute("flex", --flex);

    
    wrapPaletteItem(aItem, lastRow, lastSpacer);
  }
}

function fillRowWithFlex(aRow)
{
  var remainingFlex = kRowMax - aRow.childNodes.length;
  if (remainingFlex > 0) {
    var spacer = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                          "spacer");
    spacer.setAttribute("flex", remainingFlex);
    aRow.appendChild(spacer);
  }
}






function cleanUpItemForPalette(aItem, aWrapper)
{
  aWrapper.setAttribute("place", "palette");
  setWrapperType(aItem, aWrapper);

  if (aItem.hasAttribute("title"))
    aWrapper.setAttribute("title", aItem.getAttribute("title"));
  else if (isSpecialItem(aItem)) {
    var stringBundle = document.getElementById("stringBundle");
    var title = stringBundle.getString(aItem.id + "Title");
    aWrapper.setAttribute("title", title);
  }
  
  
  aItem.removeAttribute("command");
  aItem.removeAttribute("observes");
  aItem.removeAttribute("disabled");
  aItem.removeAttribute("type");
  
  if (aItem.localName == "toolbaritem" && aItem.firstChild) {
    aItem.firstChild.removeAttribute("observes");

    
    
    aItem.firstChild.removeAttribute("busy");
  }
}







function cleanupItemForToolbar(aItem, aWrapper)
{
  setWrapperType(aItem, aWrapper);
  aWrapper.setAttribute("place", "toolbar");

  if (aItem.hasAttribute("command")) {
    aWrapper.setAttribute("itemcommand", aItem.getAttribute("command"));
    aItem.removeAttribute("command");
  }

  if (aItem.disabled) {
    aWrapper.setAttribute("itemdisabled", "true");
    aItem.disabled = false;
  }
}

function setWrapperType(aItem, aWrapper)
{
  if (aItem.localName == "toolbarseparator") {
    aWrapper.setAttribute("type", "separator");
  } else if (aItem.localName == "toolbarspring") {
    aWrapper.setAttribute("type", "spring");
  } else if (aItem.localName == "toolbarspacer") {
    aWrapper.setAttribute("type", "spacer");
  } else if (aItem.localName == "splitter") {
    aWrapper.setAttribute("type", "splitter");
  } else if (aItem.localName == "toolbaritem" && aItem.firstChild) {
    aWrapper.setAttribute("type", aItem.firstChild.localName);
  }
}

function setDragActive(aItem, aValue)
{
  var node = aItem;
  var direction = window.getComputedStyle(aItem, null).direction;
  var value = direction == "ltr"? "left" : "right";
  if (aItem.localName == "toolbar") {
    node = aItem.lastChild;
    value = direction == "ltr"? "right" : "left";
  }
  
  if (!node)
    return;
  
  if (aValue) {
    if (!node.hasAttribute("dragover"))
      node.setAttribute("dragover", value);
  } else {
    node.removeAttribute("dragover");
  }
}

function addNewToolbar()
{
  var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                .getService(Components.interfaces.nsIPromptService);

  var stringBundle = document.getElementById("stringBundle");
  var message = stringBundle.getString("enterToolbarName");
  var title = stringBundle.getString("enterToolbarTitle");

  var name = {};

  while (true) {

    if (!promptService.prompt(window, title, message, name, null, {}))
      return;
    
    if (!name.value) {
      message = stringBundle.getFormattedString("enterToolbarBlank", [name.value]);
      continue;
    }

    var dupeFound = false;

     
    for (i = 0; i < gToolbox.childNodes.length; ++i) {
      var toolbar = gToolbox.childNodes[i];
      var toolbarName = toolbar.getAttribute("toolbarname");

      if (toolbarName == name.value &&
          toolbar.getAttribute("type") != "menubar" &&
          toolbar.nodeName == 'toolbar') {
        dupeFound = true;
        break;
      }
    }

    if (!dupeFound)
      break;

    message = stringBundle.getFormattedString("enterToolbarDup", [name.value]);
  }
    
  gToolbox.appendCustomToolbar(name.value, "");
  
  gToolboxChanged = true;
}





function restoreDefaultSet()
{
  
  
  var savedAttributes = saveItemAttributes(["itemdisabled", "itemcommand"]);

  
  var toolbar = gToolbox.firstChild;
  while (toolbar) {
    if (isCustomizableToolbar(toolbar)) {
      if (!toolbar.hasAttribute("customindex")) {
        var defaultSet = toolbar.getAttribute("defaultset");
        if (defaultSet)
          toolbar.currentSet = defaultSet;
      }
    }
    toolbar = toolbar.nextSibling;
  }

  
  var defaultMode = gToolbox.getAttribute("defaultmode");
  var defaultIconsSmall = gToolbox.getAttribute("defaulticonsize") == "small";

  updateIconSize(defaultIconsSmall);
  document.getElementById("smallicons").checked = defaultIconsSmall;
  updateToolbarMode(defaultMode);
  document.getElementById("modelist").value = defaultMode;
  
  
  var child = gToolbox.lastChild;
  while (child) {
    if (child.hasAttribute("customindex")) {
      var thisChild = child;
      child = child.previousSibling;
      gToolbox.removeChild(thisChild);
    } else {
      child = child.previousSibling;
    }
  }
  
  
  buildPalette();

  
  wrapToolbarItems();

  
  restoreItemAttributes(["itemdisabled", "itemcommand"], savedAttributes);

  gToolboxChanged = true;
}

function saveItemAttributes(aAttributeList)
{
  var items = [];
  var paletteItems = gToolbox.getElementsByTagName("toolbarpaletteitem");
  for (var i = 0; i < paletteItems.length; i++) {
    var paletteItem = paletteItems.item(i);
    for (var j = 0; j < aAttributeList.length; j++) {
      var attr = aAttributeList[j];
      if (paletteItem.hasAttribute(attr)) {
        items.push([paletteItem.id, attr, paletteItem.getAttribute(attr)]);
      }
    }
  }
  return items;
}

function restoreItemAttributes(aAttributeList, aSavedAttrList)
{
  var paletteItems = gToolbox.getElementsByTagName("toolbarpaletteitem");

  for (var i = 0; i < paletteItems.length; i++) {
    var paletteItem = paletteItems.item(i);

    
    
    for (var j = 0; j < aAttributeList.length; j++)
      paletteItem.removeAttribute(aAttributeList[j]);

    for (var j = 0; j < aSavedAttrList.length; j++) {
      var savedAttr = aSavedAttrList[j];
      if (paletteItem.id == savedAttr[0]) {
        paletteItem.setAttribute(savedAttr[1], savedAttr[2]);
      }
    }
  }
}

function updateIconSize(aUseSmallIcons)
{
  gToolboxIconSize = aUseSmallIcons ? "small" : "large";
  
  setAttribute(gToolbox, "iconsize", gToolboxIconSize);
  gToolboxDocument.persist(gToolbox.id, "iconsize");
  
  for (var i = 0; i < gToolbox.childNodes.length; ++i) {
    var toolbar = getToolbarAt(i);
    if (isCustomizableToolbar(toolbar)) {
      setAttribute(toolbar, "iconsize", gToolboxIconSize);
      gToolboxDocument.persist(toolbar.id, "iconsize");
    }
  }
}

function updateToolbarMode(aModeValue)
{
  setAttribute(gToolbox, "mode", aModeValue);
  gToolboxDocument.persist(gToolbox.id, "mode");

  for (var i = 0; i < gToolbox.childNodes.length; ++i) {
    var toolbar = getToolbarAt(i);
    if (isCustomizableToolbar(toolbar)) {
      setAttribute(toolbar, "mode", aModeValue);
      gToolboxDocument.persist(toolbar.id, "mode");
    }
  }

  var iconSizeCheckbox = document.getElementById("smallicons");
  iconSizeCheckbox.disabled = aModeValue == "text";
}


function setAttribute(aElt, aAttr, aVal)
{
 if (aVal)
    aElt.setAttribute(aAttr, aVal);
  else
    aElt.removeAttribute(aAttr);
}

function isCustomizableToolbar(aElt)
{
  return aElt.localName == "toolbar" &&
         aElt.getAttribute("customizable") == "true";
}

function isSpecialItem(aElt)
{
  return aElt.localName == "toolbarseparator" ||
         aElt.localName == "toolbarspring" ||
         aElt.localName == "toolbarspacer" ||
         aElt.localName == "splitter";
}

function isToolbarItem(aElt)
{
  return aElt.localName == "toolbarbutton" ||
         aElt.localName == "toolbaritem" ||
         aElt.localName == "toolbarseparator" ||
         aElt.localName == "toolbarspring" ||
         aElt.localName == "toolbarspacer" ||
         aElt.localName == "splitter";
}




function onToolbarDragGesture(aEvent)
{
  nsDragAndDrop.startDrag(aEvent, dragStartObserver);
}

function onToolbarDragOver(aEvent)
{
  nsDragAndDrop.dragOver(aEvent, toolbarDNDObserver);
}

function onToolbarDragDrop(aEvent)
{
  nsDragAndDrop.drop(aEvent, toolbarDNDObserver);
}

function onToolbarDragExit(aEvent)
{
  if (gCurrentDragOverItem)
    setDragActive(gCurrentDragOverItem, false);
}

var dragStartObserver =
{
  onDragStart: function (aEvent, aXferData, aDragAction) {
    var documentId = gToolboxDocument.documentElement.id;
    
    var item = aEvent.target;
    while (item && item.localName != "toolbarpaletteitem")
      item = item.parentNode;
    
    item.setAttribute("dragactive", "true");
    
    aXferData.data = new TransferDataSet();
    var data = new TransferData();
    data.addDataForFlavour("text/toolbarwrapper-id/"+documentId, item.firstChild.id);
    aXferData.data.push(data);
    aDragAction.action = Components.interfaces.nsIDragService.DRAGDROP_ACTION_MOVE;
  }
}

var toolbarDNDObserver =
{
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    var toolbar = aEvent.target;
    var dropTarget = aEvent.target;
    while (toolbar && toolbar.localName != "toolbar") {
      dropTarget = toolbar;
      toolbar = toolbar.parentNode;
    }
    
    var previousDragItem = gCurrentDragOverItem;

    
    if (!isCustomizableToolbar(toolbar)) {
      gCurrentDragOverItem = null;
      return;
    }
    
    if (dropTarget.localName == "toolbar") {
      gCurrentDragOverItem = dropTarget;
    } else {
      gCurrentDragOverItem = null;

      var direction = window.getComputedStyle(dropTarget.parentNode, null).direction;
      var dropTargetCenter = dropTarget.boxObject.x + (dropTarget.boxObject.width / 2);
      if (direction == "ltr")
        dragAfter = aEvent.clientX > dropTargetCenter;
      else
        dragAfter = aEvent.clientX < dropTargetCenter;
        
      if (dragAfter) {
        gCurrentDragOverItem = dropTarget.nextSibling;
        if (!gCurrentDragOverItem)
          gCurrentDragOverItem = toolbar;
      } else
        gCurrentDragOverItem = dropTarget;
    }    

    if (previousDragItem && gCurrentDragOverItem != previousDragItem) {
      setDragActive(previousDragItem, false);
    }
    
    setDragActive(gCurrentDragOverItem, true);
    
    aDragSession.canDrop = true;
  },
  
  onDrop: function (aEvent, aXferData, aDragSession)
  {
    if (!gCurrentDragOverItem)
      return;
    
    setDragActive(gCurrentDragOverItem, false);

    var draggedItemId = aXferData.data;
    if (gCurrentDragOverItem.id == draggedItemId)
      return;

    var toolbar = aEvent.target;
    while (toolbar.localName != "toolbar")
      toolbar = toolbar.parentNode;

    var draggedPaletteWrapper = document.getElementById("wrapper-"+draggedItemId);       
    if (!draggedPaletteWrapper) {
      
      
      
      
      var wrapper = gToolboxDocument.getElementById("wrapper-"+draggedItemId);
      if (wrapper == gCurrentDragOverItem)
        return;

      
      if (wrapper.parentNode.firstPermanentChild && wrapper.parentNode.firstPermanentChild.id == wrapper.firstChild.id)
        return;
      if (wrapper.parentNode.lastPermanentChild && wrapper.parentNode.lastPermanentChild.id == wrapper.firstChild.id)
        return;

      
      wrapper.parentNode.removeChild(wrapper);

      
      var dropToolbar = null;
      if (gCurrentDragOverItem.localName == "toolbar")
        dropToolbar = gCurrentDragOverItem;
      else
        dropToolbar = gCurrentDragOverItem.parentNode;
      
      
      if (gCurrentDragOverItem != dropToolbar)
        dropToolbar.insertBefore(wrapper, gCurrentDragOverItem);
      else
        dropToolbar.appendChild(wrapper);
    } else {
      
      
      
      var wrapper = createWrapper("");
      gToolboxDocument.adoptNode(wrapper);

      
      var newItem = toolbar.insertItem(draggedItemId, gCurrentDragOverItem == toolbar ? null : gCurrentDragOverItem, wrapper);
      
      
      cleanupItemForToolbar(newItem, wrapper);
      wrapper.id = "wrapper-"+newItem.id;
      wrapper.flex = newItem.flex;

      
      var currentRow = draggedPaletteWrapper.parentNode;
      if (draggedItemId != "separator" &&
          draggedItemId != "spring" &&
          draggedItemId != "spacer" &&
          draggedItemId != "splitter")
      {
        currentRow.removeChild(draggedPaletteWrapper);

        while (currentRow) {
          
          
          var nextRow = currentRow.nextSibling;
          
          if (!nextRow) {
            var last = currentRow.lastChild;
            var first = currentRow.firstChild;
            if (first == last) {
              
              currentRow.parentNode.removeChild(currentRow);
              break;
            }

            if (last.localName == "spacer") {
              var flex = last.getAttribute("flex");
              last.setAttribute("flex", ++flex);
              
              last.hidden = true;
              last.hidden = false;
              break;
            } else {
              
              var spacer = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
                                                    "spacer");
              spacer.setAttribute("flex", "1");
              currentRow.appendChild(spacer);
            }
            break;
          }
          
          currentRow.appendChild(nextRow.firstChild);
          currentRow = currentRow.nextSibling;
        }
      }
    }
    
    gCurrentDragOverItem = null;

    gToolboxChanged = true;
  },
  
  _flavourSet: null,
  
  getSupportedFlavours: function ()
  {
    if (!this._flavourSet) {
      this._flavourSet = new FlavourSet();
      var documentId = gToolboxDocument.documentElement.id;
      this._flavourSet.appendFlavour("text/toolbarwrapper-id/"+documentId);
    }
    return this._flavourSet;
  }
}

var paletteDNDObserver =
{
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    aDragSession.canDrop = true;
  },
  
  onDrop: function(aEvent, aXferData, aDragSession)
  {
    var itemId = aXferData.data;
    
    var wrapper = gToolboxDocument.getElementById("wrapper-"+itemId);
    if (wrapper) {
      
      if (wrapper.parentNode.firstPermanentChild && wrapper.parentNode.firstPermanentChild.id == wrapper.firstChild.id)
        return;
      if (wrapper.parentNode.lastPermanentChild && wrapper.parentNode.lastPermanentChild.id == wrapper.firstChild.id)
        return;

      
      wrapper.parentNode.removeChild(wrapper);
      
      var wrapperType = wrapper.getAttribute("type");
      if (wrapperType != "separator" &&
          wrapperType != "spacer" &&
          wrapperType != "spring" &&
          wrapperType != "splitter") {
        
        var templateNode = gToolbox.palette.firstChild;
        while (templateNode) {
          if (templateNode.id == itemId)
            break;
          templateNode = templateNode.nextSibling;
        }
        if (!templateNode)
          return;
        
        
        var paletteItem = templateNode.cloneNode(true);
        appendPaletteItem(paletteItem);
      }
    }
    
    gToolboxChanged = true;
  },
  
  _flavourSet: null,
  
  getSupportedFlavours: function ()
  {
    if (!this._flavourSet) {
      this._flavourSet = new FlavourSet();
      var documentId = gToolboxDocument.documentElement.id;
      this._flavourSet.appendFlavour("text/toolbarwrapper-id/"+documentId);
    }
    return this._flavourSet;
  }
}

