





































function BuildJSEAttributeNameList()
{
  gDialog.AddJSEAttributeNameList.removeAllItems();
  
  
  var elementName = gElement.localName.toLowerCase();
  if (elementName in gJSAttr)
  {
    var attNames = gJSAttr[elementName];
    var i;
    var popup;
    var sep;

    if (attNames && attNames.length)
    {
      
      
      if (attNames[0] == "noJSEvents")
      {
        var tab = document.getElementById("tabJSE");
        if (tab)
          tab.parentNode.removeChild(tab);

        return;
      }

      for (i = 0; i < attNames.length; i++)
        gDialog.AddJSEAttributeNameList.appendItem(attNames[i], attNames[i]);

      popup = gDialog.AddJSEAttributeNameList.firstChild;
      if (popup)
      {
        sep = document.createElementNS(XUL_NS, "menuseparator");
        if (sep)
          popup.appendChild(sep);
      }        
    }
  }

  
  for (i = 0; i < gCoreJSEvents.length; i++)
  {
    if (gCoreJSEvents[i] == "-")
    {
      if (!popup)
        popup = gDialog.AddJSEAttributeNameList.firstChild;

      sep = document.createElementNS(XUL_NS, "menuseparator");

      if (popup && sep)
        popup.appendChild(sep);
    }
    else
      gDialog.AddJSEAttributeNameList.appendItem(gCoreJSEvents[i], gCoreJSEvents[i]);
  }
  
  gDialog.AddJSEAttributeNameList.selectedIndex = 0;

  
  onSelectJSETreeItem();
}


function BuildJSEAttributeTable()
{
  var nodeMap = gElement.attributes;
  if (nodeMap.length > 0)
  {
    var added = false;
    for (var i = 0; i < nodeMap.length; i++)
    {
      if( CheckAttributeNameSimilarity( nodeMap[i].nodeName, JSEAttrs ) )
        continue;   
      if( !IsEventHandler( nodeMap[i].nodeName ) )
        continue; 
      var name  = nodeMap[i].nodeName.toLowerCase();
      var value = gElement.getAttribute(nodeMap[i].nodeName);
      if (AddTreeItem( name, value, "JSEAList", JSEAttrs )) 
        added = true;
    }

    
    if (added)
      gDialog.AddJSEAttributeTree.selectedIndex = 0;
  }
}


function IsEventHandler( which )
{
  var handlerName = which.toLowerCase();
  var firstTwo = handlerName.substring(0,2);
  if (firstTwo == "on")
    return true;
  else
    return false;
}

function onSelectJSEAttribute()
{
  if(!gDoOnSelectTree)
    return;

  gDialog.AddJSEAttributeValueInput.value = 
      GetAndSelectExistingAttributeValue(gDialog.AddJSEAttributeNameList.label, "JSEAList");
}

function onSelectJSETreeItem()
{
  var tree = gDialog.AddJSEAttributeTree;
  if (tree && tree.view.selection.count)
  {
    
    gDialog.AddJSEAttributeNameList.value = GetTreeItemAttributeStr(getSelectedItem(tree));

    
    gUpdateTreeValue = false;
    gDialog.AddJSEAttributeValueInput.value =  GetTreeItemValueStr(getSelectedItem(tree));
    gUpdateTreeValue = true;
  }
}

function onInputJSEAttributeValue()
{
  if (gUpdateTreeValue)
  {

    var name = TrimString(gDialog.AddJSEAttributeNameList.label);
    var value = TrimString(gDialog.AddJSEAttributeValueInput.value);

    
    
    
    if (!UpdateExistingAttribute( name, value, "JSEAList" ) && value)
      AddTreeItem( name, value, "JSEAList", JSEAttrs );
  }
}

function editJSEAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    gDialog.AddJSEAttributeValueInput.inputField.select();
}

function UpdateJSEAttributes()
{
  var JSEAList = document.getElementById("JSEAList");
  var i;

  
  for (i = 0; i < JSERAttrs.length; i++)
  {
    var name = JSERAttrs[i];

    if (gElement.hasAttribute(name))
      doRemoveAttribute(name);
  }

  
  for (i = 0; i < JSEAList.childNodes.length; i++)
  {
    var item = JSEAList.childNodes[i];

    
    doSetAttribute( GetTreeItemAttributeStr(item), GetTreeItemValueStr(item) );
  }
}

function RemoveJSEAttribute()
{
  var treechildren = gDialog.AddJSEAttributeTree.lastChild;

  
  
  
  
  var newIndex = gDialog.AddJSEAttributeTree.selectedIndex;

  
  if (gDialog.AddJSEAttributeTree.view.selection.count)
  {
    var item = getSelectedItem(gDialog.AddJSEAttributeTree);

    
    var attr = GetTreeItemAttributeStr(item);

    
    if (newIndex >= (JSEAttrs.length-1))
      newIndex--;

    
    JSERAttrs[JSERAttrs.length] = attr;
    RemoveNameFromAttArray(attr, JSEAttrs);

    
    treechildren.removeChild (item);

    
    gDialog.AddJSEAttributeTree.selectedIndex = newIndex;
  }
}
