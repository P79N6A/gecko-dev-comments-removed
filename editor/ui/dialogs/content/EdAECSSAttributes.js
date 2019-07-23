









































function BuildCSSAttributeTable()
{
  var style = gElement.style;
  if (style == undefined)
  {
    dump("Inline styles undefined\n");
    return;
  }

  var declLength = style.length;

  if (declLength == undefined || declLength == 0)
  {
    if (declLength == undefined) {
      dump("Failed to query the number of inline style declarations\n");
    }

    return;
  }

  if (declLength > 0)
  {
    for (var i = 0; i < declLength; ++i)
    {
      var name = style.item(i);
      var value = style.getPropertyValue(name);
      AddTreeItem( name, value, "CSSAList", CSSAttrs );
    }
  }

  ClearCSSInputWidgets();
}

function onChangeCSSAttribute()
{
  var name = TrimString(gDialog.AddCSSAttributeNameInput.value);
  if ( !name )
    return;

  var value = TrimString(gDialog.AddCSSAttributeValueInput.value);

  
  
  if ( !UpdateExistingAttribute( name, value, "CSSAList" ) && value)
    AddTreeItem( name, value, "CSSAList", CSSAttrs );
}

function ClearCSSInputWidgets()
{
  gDialog.AddCSSAttributeTree.view.selection.clearSelection();
  gDialog.AddCSSAttributeNameInput.value ="";
  gDialog.AddCSSAttributeValueInput.value = "";
  SetTextboxFocus(gDialog.AddCSSAttributeNameInput);
}

function onSelectCSSTreeItem()
{
  if (!gDoOnSelectTree)
    return;

  var tree = gDialog.AddCSSAttributeTree;
  if (tree && tree.view.selection.count)
  {
    gDialog.AddCSSAttributeNameInput.value = GetTreeItemAttributeStr(getSelectedItem(tree));
    gDialog.AddCSSAttributeValueInput.value = GetTreeItemValueStr(getSelectedItem(tree));
  }
}

function onInputCSSAttributeName()
{
  var attName = TrimString(gDialog.AddCSSAttributeNameInput.value).toLowerCase();
  var newValue = "";

  var existingValue = GetAndSelectExistingAttributeValue(attName, "CSSAList");
  if (existingValue)
    newValue = existingValue;

  gDialog.AddCSSAttributeValueInput.value = newValue;
}

function editCSSAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    gDialog.AddCSSAttributeValueInput.inputField.select();
}

function UpdateCSSAttributes()
{
  var CSSAList = document.getElementById("CSSAList");
  var styleString = "";
  for(var i = 0; i < CSSAList.childNodes.length; i++)
  {
    var item = CSSAList.childNodes[i];
    var name = GetTreeItemAttributeStr(item);
    var value = GetTreeItemValueStr(item);
    
    
    
    if (name.indexOf(":") != -1)
      name = name.substring(0,name.lastIndexOf(":"));
    if (value.indexOf(";") != -1)
      value = value.substring(0,value.lastIndexOf(";"));
    if (i == (CSSAList.childNodes.length - 1))
      styleString += name + ": " + value + ";";   
    else
      styleString += name + ": " + value + "; ";
  }
  if (styleString)
  {
    
    doRemoveAttribute("style");
    doSetAttribute("style", styleString);  
  } 
  else if (gElement.getAttribute("style"))
    doRemoveAttribute("style");
}

function RemoveCSSAttribute()
{
  var treechildren = gDialog.AddCSSAttributeTree.lastChild;

  
  if (gDialog.AddCSSAttributeTree.view.selection.count)
  {
    var item = getSelectedItem(gDialog.AddCSSAttributeTree);

    
    
    
    treechildren.removeChild (item);

    ClearCSSInputWidgets();
  }
}

function SelectCSSTree( index )
{
  gDoOnSelectTree = false;
  try {
    gDialog.AddCSSAttributeTree.selectedIndex = index;
  } catch (e) {}
  gDoOnSelectTree = true;
}
