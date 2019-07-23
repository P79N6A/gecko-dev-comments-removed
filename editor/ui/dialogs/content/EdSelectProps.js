





































var atomService = Components.classes["@mozilla.org/atom-service;1"]
                            .getService(Components.interfaces.nsIAtomService);
var checkedAtoms = {
  "false":  atomService.getAtom("checked-false"),
  "true":   atomService.getAtom("checked-true")};

var hasValue;
var oldValue;
var insertNew;
var itemArray;
var treeBoxObject;
var treeSelection;
var selectElement;
var currentItem = null;
var selectedOption = null;
var selectedOptionCount = 0;



function getParentIndex(index)
{
  switch (itemArray[index].level)
  {
  case 0: return -1;
  case 1: return 0;
  }
  while (itemArray[--index].level > 1);
  return index;
}

function UpdateSelectMultiple()
{
  if (selectedOptionCount > 1)
  {
    gDialog.selectMultiple.checked = true;
    gDialog.selectMultiple.disabled = true;
  }
  else
    gDialog.selectMultiple.disabled = false;
}




















function optionObject(option, level)
{
  
  if (option.hasAttribute("selected"))
    selectedOptionCount++;
  this.level = level;
  this.element = option;
}

optionObject.prototype.container = false;

optionObject.prototype.getCellText = function getCellText(column)
{
  if (column.id == "SelectSelCol")
    return "";
  if (column.id == "SelectValCol" && this.element.hasAttribute("value"))
    return this.element.getAttribute("value");
  return this.element.text;
}

optionObject.prototype.cycleCell = function cycleCell(index)
{
  if (this.element.hasAttribute("selected"))
  {
    this.element.removeAttribute("selected");
    selectedOptionCount--;
    selectedOption = null;
  }
  else
  {
    
    if (gDialog.selectMultiple.checked || !selectedOption)
      selectedOptionCount++;
    else if (selectedOption)
    {
      selectedOption.removeAttribute("selected");
      var column = treeBoxObject.columns["SelectSelCol"];
      treeBoxObject.invalidateColumn(column);
      selectedOption = null;
    }
    this.element.setAttribute("selected", "");
    selectedOption = this.element;
    var column = treeBoxObject.columns["SelectSelCol"];
    treeBoxObject.invalidateCell(index, column);
  }
  if (currentItem == this)
    
    gDialog.optionSelected.setAttribute("checked", this.element.hasAttribute("selected"));
  UpdateSelectMultiple();
};

optionObject.prototype.onFocus = function onFocus()
{
  gDialog.optionText.value = this.element.text;
  hasValue = this.element.hasAttribute("value");
  oldValue = this.element.value;
  gDialog.optionHasValue.checked = hasValue;
  gDialog.optionValue.value = hasValue ? this.element.value : this.element.text;
  gDialog.optionSelected.checked = this.element.hasAttribute("selected");
  gDialog.optionDisabled.checked = this.element.hasAttribute("disabled");
  gDialog.selectDeck.setAttribute("selectedIndex", "2");
};

optionObject.prototype.onBlur = function onBlur()
{
  this.element.text = gDialog.optionText.value;
  if (gDialog.optionHasValue.checked)
    this.element.value = gDialog.optionValue.value;
  else
    this.element.removeAttribute("value");
  if (gDialog.optionSelected.checked)
    this.element.setAttribute("selected", "");
  else
    this.element.removeAttribute("selected");
  if (gDialog.optionDisabled.checked)
    this.element.setAttribute("disabled", "");
  else
    this.element.removeAttribute("disabled");
};

optionObject.prototype.canDestroy = function canDestroy(prompt)
{
  return true;




};

optionObject.prototype.destroy = function destroy()
{
  
  if (this.element.hasAttribute("selected"))
  {
    selectedOptionCount--;
    selectedOption = null;
    UpdateSelectMultiple();
  }
};














optionObject.prototype.moveUp = function moveUp()
{
  var i;
  var index = treeSelection.currentIndex;
  if (itemArray[index].level < itemArray[index - 1].level + itemArray[index - 1].container)
  {
    
    treeBoxObject.invalidateRange(getParentIndex(index), index);
    
    itemArray[index].level = 2;
    treeBoxObject.view.selectionChanged();
  }
  else
  {
    
    itemArray[index].level = itemArray[index - 1].level;
    
    itemArray.splice(index, 0, itemArray.splice(--index, 1)[0]);
  }
  selectTreeIndex(index, true);
}

optionObject.prototype.canMoveDown = function canMoveDown()
{
  
  return this.level > 1 || itemArray.length - treeSelection.currentIndex > 1;
}

optionObject.prototype.moveDown = function moveDown()
{
  var i;
  var index = treeSelection.currentIndex;
  if (index + 1 == itemArray.length || itemArray[index].level > itemArray[index + 1].level)
  {
    
    treeBoxObject.invalidateRange(getParentIndex(index), index);
    
    itemArray[index].level = 1;
    treeBoxObject.view.selectionChanged();
  }
  else
  {
    
    itemArray[index].level += itemArray[index + 1].container;
    
    itemArray.splice(index, 0, itemArray.splice(++index, 1)[0]);
  }
  selectTreeIndex(index, true);
}

optionObject.prototype.appendOption = function appendOption(child, parent)
{
  
  if (this.level == 1)
    return gDialog.appendOption(child, 0);

  
  parent = getParentIndex(parent);
  return itemArray[parent].appendOption(child, parent);
};



function optgroupObject(optgroup)
{
  this.element = optgroup;
}

optgroupObject.prototype.level = 1;

optgroupObject.prototype.container = true;

optgroupObject.prototype.getCellText = function getCellText(column)
{
  return column.id == "SelectTextCol" ? this.element.label : "";
}

optgroupObject.prototype.cycleCell = function cycleCell(index)
{
};

optgroupObject.prototype.onFocus = function onFocus()
{
  gDialog.optgroupLabel.value = this.element.label;
  gDialog.optgroupDisabled.checked = this.element.disabled;
  gDialog.selectDeck.setAttribute("selectedIndex", "1");
};

optgroupObject.prototype.onBlur = function onBlur()
{
  this.element.label = gDialog.optgroupLabel.value;
  this.element.disabled = gDialog.optgroupDisabled.checked;
};

optgroupObject.prototype.canDestroy = function canDestroy(prompt)
{
  
  return gDialog.nextChild(treeSelection.currentIndex) - treeSelection.currentIndex == 1;





};

optgroupObject.prototype.destroy = function destroy()
{
};

optgroupObject.prototype.moveUp = function moveUp()
{
  
  var index = treeSelection.currentIndex;
  var i = index;
  while (itemArray[--index].level > 1);
  var j = gDialog.nextChild(i);
  
  var movedItems = itemArray.splice(i, j - i);
  var endItems = itemArray.splice(index);
  itemArray = itemArray.concat(movedItems).concat(endItems);
  
  treeBoxObject.invalidateRange(index, j);
  selectTreeIndex(index, true);
}

optgroupObject.prototype.canMoveDown = function canMoveDown()
{
  return gDialog.lastChild() > treeSelection.currentIndex;
}

optgroupObject.prototype.moveDown = function moveDown()
{
  
  var index = treeSelection.currentIndex;
  var i = gDialog.nextChild(index);
  var j = gDialog.nextChild(i);
  
  var movedItems = itemArray.splice(i, j - 1);
  var endItems = itemArray.splice(index);
  itemArray = itemArray.concat(movedItems).concat(endItems);
  
  treeBoxObject.invalidateRange(index, j);
  index += j - i;
  selectTreeIndex(index, true);
}

optgroupObject.prototype.appendOption = function appendOption(child, parent)
{
  var index = gDialog.nextChild(parent);
  
  var primaryCol = treeBoxObject.getPrimaryColumn();
  treeBoxObject.invalidateCell(index - 1, primaryCol);
  
  itemArray.splice(index, 0, new optionObject(child, 2));
  treeBoxObject.rowCountChanged(index, 1);
  selectTreeIndex(index, false);
};



function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    dump("Failed to get active editor!\n");
    window.close();
    return;
  }

  
  const kTagName = "select";
  try {
    selectElement = editor.getSelectedElement(kTagName);
  } catch (e) {}

  if (selectElement)
    
    insertNew = false;
  else
  {
    insertNew = true;

    
    
    try {
      selectElement = editor.createElementWithDefaults(kTagName);
    } catch (e) {}

    if(!selectElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
  }

  
  gDialog = {
    
    accept:           document.documentElement.getButton("accept"),
    selectDeck:       document.getElementById("SelectDeck"),
    selectName:       document.getElementById("SelectName"),
    selectSize:       document.getElementById("SelectSize"),
    selectMultiple:   document.getElementById("SelectMultiple"),
    selectDisabled:   document.getElementById("SelectDisabled"),
    selectTabIndex:   document.getElementById("SelectTabIndex"),
    optgroupLabel:    document.getElementById("OptGroupLabel"),
    optgroupDisabled: document.getElementById("OptGroupDisabled"),
    optionText:       document.getElementById("OptionText"),
    optionHasValue:   document.getElementById("OptionHasValue"),
    optionValue:      document.getElementById("OptionValue"),
    optionSelected:   document.getElementById("OptionSelected"),
    optionDisabled:   document.getElementById("OptionDisabled"),
    removeButton:     document.getElementById("RemoveButton"),
    previousButton:   document.getElementById("PreviousButton"),
    nextButton:       document.getElementById("NextButton"),
    tree:             document.getElementById("SelectTree"),
    
    element:          selectElement.cloneNode(false),
    level:            0,
    container:        true,
    getCellText:      function getCellText(column)
    {
      return column.id == "SelectTextCol" ? this.element.getAttribute("name") : "";
    },
    cycleCell:        function cycleCell(index) {},
    onFocus:          function onFocus()
    {
      gDialog.selectName.value = this.element.getAttribute("name");
      gDialog.selectSize.value = this.element.getAttribute("size");
      gDialog.selectMultiple.checked = this.element.hasAttribute("multiple");
      gDialog.selectDisabled.checked = this.element.hasAttribute("disabled");
      gDialog.selectTabIndex.value = this.element.getAttribute("tabindex");
      this.selectDeck.setAttribute("selectedIndex", "0");
      onNameInput();
    },
    onBlur:           function onBlur()
    {
      this.element.setAttribute("name", gDialog.selectName.value);
      if (gDialog.selectSize.value)
        this.element.setAttribute("size", gDialog.selectSize.value);
      else
        this.element.removeAttribute("size");
      if (gDialog.selectMultiple.checked)
        this.element.setAttribute("multiple", "");
      else
        this.element.removeAttribute("multiple");
      if (gDialog.selectDisabled.checked)
        this.element.setAttribute("disabled", "");
      else
        this.element.removeAttribute("disabled");
      if (gDialog.selectTabIndex.value)
        this.element.setAttribute("tabindex", gDialog.selectTabIndex.value);
      else
        this.element.removeAttribute("tabindex");
    },
    appendOption:     function appendOption(child, parent)
    {
      var index = itemArray.length;
      
      treeBoxObject.invalidateRange(this.lastChild(), index);
      
      itemArray.push(new optionObject(child, 1));
      treeBoxObject.rowCountChanged(index, 1);
      selectTreeIndex(index, false);
    },
    canDestroy:       function canDestroy(prompt)
    {
      return false;
    },
    canMoveDown:      function canMoveDown()
    {
      return false;
    },
    
    
    nextChild:        function nextChild(index)
    {
      while (++index < itemArray.length && itemArray[index].level > 1);
      return index;
    },
    
    lastChild:        function lastChild()
    {
      var index = itemArray.length;
      while (itemArray[--index].level > 1);
      return index;
    }
  }
  
  itemArray = [gDialog];

  
  for (var child = selectElement.firstChild; child; child = child.nextSibling)
  {
    if (child.tagName == "OPTION")
      itemArray.push(new optionObject(child.cloneNode(true), 1));
    else if (child.tagName == "OPTGROUP")
    {
      itemArray.push(new optgroupObject(child.cloneNode(false)));
      for (var grandchild = child.firstChild; grandchild; grandchild = grandchild.nextSibling)
        if (grandchild.tagName == "OPTION")
          itemArray.push(new optionObject(grandchild.cloneNode(true), 2));
    }
  }

  UpdateSelectMultiple();

  
  treeBoxObject = gDialog.tree.treeBoxObject;
  treeBoxObject.view = {
    QueryInterface : function QueryInterface(aIID)
    {
      if (aIID.equals(Components.interfaces.nsITreeView) ||
          aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
          aIID.equals(Components.interfaces.nsISupports))
        return this;

      Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
      return null;
    },
    
    get wrappedJSObject() { return this; },
    get rowCount() { return itemArray.length; },
    get selection() { return treeSelection; },
    set selection(selection) { return treeSelection = selection; },
    getRowProperties: function getRowProperties(index, column, prop) { },
    
    getCellProperties: function getCellProperties(index, column, prop)
    {
      if (column.id == "SelectSelCol" && !itemArray[index].container)
        prop.AppendElement(checkedAtoms[itemArray[index].element.hasAttribute("selected")]);
    },
    getColumnProperties: function getColumnProperties(column, prop) { },
    
    isContainer: function isContainer(index) { return itemArray[index].container; },
    isContainerOpen: function isContainerOpen(index) { return true; },
    isContainerEmpty: function isContainerEmpty(index) { return true; },
    isSeparator: function isSeparator(index) { return false; },
    isSorted: function isSorted() { return false; },
    
    canDrop: function canDrop(index, orientation) { return false; },
    drop: function drop(index, orientation) { alert('drop:' + index + ',' + orientation); },
    
    getParentIndex: getParentIndex,
    
    hasNextSibling: function hasNextSibling(index, after)
    {
      if (!index)
        return false;
      var level = itemArray[index].level;
      while (++after < itemArray.length)
        switch (level - itemArray[after].level)
        {
        case 1: return false;
        case 0: return true;
        }
      return false;
    },
    getLevel: function getLevel(index) { return itemArray[index].level; },
    getImageSrc: function getImageSrc(index, column) { },
    getProgressMode : function getProgressMode(index,column) { },
    getCellValue: function getCellValue(index, column) { },
    getCellText: function getCellText(index, column) { return itemArray[index].getCellText(column); },
    setTree: function setTree(tree) { this.tree = tree; },
    toggleOpenState: function toggleOpenState(index) { },
    cycleHeader: function cycleHeader(col) { },
    selectionChanged: function selectionChanged()
    {
      
      if (currentItem)
        currentItem.onBlur();
      var currentIndex = treeSelection.currentIndex;
      currentItem = itemArray[currentIndex];
      gDialog.removeButton.disabled = !currentItem.canDestroy();
      gDialog.previousButton.disabled = currentIndex < 2;
      gDialog.nextButton.disabled = !currentItem.canMoveDown();
      
      globalElement = currentItem.element;
      currentItem.onFocus();
    },
    cycleCell: function cycleCell(index, column) { itemArray[index].cycleCell(index); },
    isEditable: function isEditable(index, column) { return false; },
    isSelectable: function isSelectable(index, column) { return false; },
    performAction: function performAction(action) { },
    performActionOnCell: function performActionOnCell(action, index, column) { }
  };
  treeSelection.select(0);
  currentItem = gDialog;
  

  SetTextboxFocus(gDialog.selectName);

  SetWindowLocation();
}


function InitDialog()
{
  currentItem.onFocus();
}


function ValidateData()
{
  currentItem.onBlur();
  return true;
}

function onAccept()
{
  
  
  ValidateData();

  var editor = GetCurrentEditor();

  
  editor.beginTransaction();

  try
  {
    editor.cloneAttributes(selectElement, gDialog.element);

    if (insertNew)
      
      editor.insertElementAtSelection(selectElement, true);

    editor.setShouldTxnSetSelection(false);

    while (selectElement.lastChild)
      editor.deleteNode(selectElement.lastChild);

    var offset = 0;
    for (var i = 1; i < itemArray.length; i++)
      if (itemArray[i].level > 1)
        selectElement.lastChild.appendChild(itemArray[i].element);
      else
        editor.insertNode(itemArray[i].element, selectElement, offset++, true);

    editor.setShouldTxnSetSelection(true);
  }
  finally
  {
    editor.endTransaction();
  }

  SaveWindowLocation();

  return true;
}


function AddOption()
{
  currentItem.appendOption(GetCurrentEditor().createElementWithDefaults("option"), treeSelection.currentIndex);
  SetTextboxFocus(gDialog.optionText);
}

function AddOptGroup()
{
  var optgroupElement = GetCurrentEditor().createElementWithDefaults("optgroup");
  var index = itemArray.length;
  
  treeBoxObject.invalidateRange(gDialog.lastChild(), index);
  
  itemArray.push(new optgroupObject(optgroupElement));
  treeBoxObject.rowCountChanged(index, 1);
  selectTreeIndex(index, false);
  SetTextboxFocus(gDialog.optgroupLabel);
}

function RemoveElement()
{
  if (currentItem.canDestroy(true))
  {
    
    var index = treeSelection.currentIndex;
    var level = itemArray[index].level;
    
    itemArray[index].destroy();
    itemArray.splice(index, 1);
    --index;
    
    if (level == 1) {
      var last = gDialog.lastChild();
      if (index > last)
        treeBoxObject.invalidateRange(last, index);
    }
    selectTreeIndex(index, true);
    treeBoxObject.rowCountChanged(++index, -1);
  }
}


function onTreeKeyUp(event)
{
  if (event.keyCode == event.DOM_VK_SPACE)
    currentItem.cycleCell();
}

function onNameInput()
{
  var disabled = !gDialog.selectName.value;
  if (gDialog.accept.disabled != disabled)
    gDialog.accept.disabled = disabled;
  gDialog.element.setAttribute("name", gDialog.selectName.value);
  
  var primaryCol = treeBoxObject.getPrimaryColumn();
  treeBoxObject.invalidateCell(treeSelection.currentIndex, primaryCol);
}

function onLabelInput()
{
  currentItem.element.setAttribute("label", gDialog.optgroupLabel.value);
  
  var primaryCol = treeBoxObject.getPrimaryColumn();
  treeBoxObject.invalidateCell(treeSelection.currentIndex, primaryCol);
}

function onTextInput()
{
  currentItem.element.text = gDialog.optionText.value;
  
  if (hasValue) {
    var primaryCol = treeBoxObject.getPrimaryColumn();
    treeBoxObject.invalidateCell(treeSelection.currentIndex, primaryCol);
  }
  else
  {
    gDialog.optionValue.value = gDialog.optionText.value;
    treeBoxObject.invalidateRow(treeSelection.currentIndex);
  }
}

function onValueInput()
{
  gDialog.optionHasValue.checked = hasValue = true;
  oldValue = gDialog.optionValue.value;
  currentItem.element.setAttribute("value", oldValue);
  
  var column = treeBoxObject.columns["SelectValCol"];
  treeBoxObject.invalidateCell(treeSelection.currentIndex, column);
}

function onHasValueClick()
{
  hasValue = gDialog.optionHasValue.checked;
  if (hasValue)
  {
    gDialog.optionValue.value = oldValue;
    currentItem.element.setAttribute("value", oldValue);
  }
  else
  {
    oldValue = gDialog.optionValue.value;
    gDialog.optionValue.value = gDialog.optionText.value;
    currentItem.element.removeAttribute("value");
  }
  
  var column = treeBoxObject.columns["SelectValCol"];
  treeBoxObject.invalidateCell(treeSelection.currentIndex, column);
}

function onSelectMultipleClick()
{
  
  if (!gDialog.selectMultiple.checked && selectedOptionCount == 1 && !selectedOption)
    for (var i = 1; !(selectedOption = itemArray[i].element).hasAttribute("selected"); i++);
}

function selectTreeIndex(index, focus)
{
  treeSelection.select(index);
  treeBoxObject.ensureRowIsVisible(index);
  if (focus)
    gDialog.tree.focus();
}
