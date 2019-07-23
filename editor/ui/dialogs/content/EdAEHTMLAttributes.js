





































function BuildHTMLAttributeNameList()
{
  gDialog.AddHTMLAttributeNameInput.removeAllItems();
  
  var elementName = gElement.localName.toLowerCase();
  var attNames = gHTMLAttr[elementName];

  if (attNames && attNames.length)
  {
    var menuitem;

    for (var i = 0; i < attNames.length; i++)
    {
      var name = attNames[i];
      var limitFirstChar;

      if (name == "_core")
      {
        
        for (var j = 0; j < gCoreHTMLAttr.length; j++)
        {
          name = gCoreHTMLAttr[j];

          
          
          limitFirstChar = name.indexOf("^") >= 0;
          if (limitFirstChar)
          {
            menuitem = gDialog.AddHTMLAttributeNameInput.appendItem(name.replace(/\^/g, ""));
            menuitem.setAttribute("limitFirstChar", "true");
          }
          else
            gDialog.AddHTMLAttributeNameInput.appendItem(name);
        }
      }
      else if (name == "-")
      {
        
        var popup = gDialog.AddHTMLAttributeNameInput.firstChild;
        if (popup)
        {
          var sep = document.createElementNS(XUL_NS, "menuseparator");
          if (sep)
            popup.appendChild(sep);
        }        
      }
      else
      {
        
        var forceOneChar = name.indexOf("!") >= 0;
        var forceInteger = name.indexOf("#") >= 0;
        var forceSignedInteger = name.indexOf("+") >= 0;
        var forceIntOrPercent = name.indexOf("%") >= 0;
        limitFirstChar = name.indexOf("\^") >= 0;
        

        
        name = name.replace(/[!^#%$+]/g, "");

        menuitem = gDialog.AddHTMLAttributeNameInput.appendItem(name);
        if (menuitem)
        {
          
          
          
          
          

          
          if (forceOneChar)
            menuitem.setAttribute("forceOneChar","true");
          if (limitFirstChar)
            menuitem.setAttribute("limitFirstChar", "true");
          if (forceInteger)
            menuitem.setAttribute("forceInteger", "true");
          if (forceSignedInteger)
            menuitem.setAttribute("forceSignedInteger", "true");
          if (forceIntOrPercent)
            menuitem.setAttribute("forceIntOrPercent", "true");
        }
      }
    }
  }
}


function BuildHTMLAttributeTable()
{
  var nodeMap = gElement.attributes;
  var i;
  if (nodeMap.length > 0) 
  {
    var added = false;
    for(i = 0; i < nodeMap.length; i++)
    {
      if ( CheckAttributeNameSimilarity( nodeMap[i].nodeName, HTMLAttrs ) ||
          IsEventHandler( nodeMap[i].nodeName ) ||
          TrimString( nodeMap[i].nodeName.toLowerCase() ) == "style" ) {
        continue;   
      }
      var name  = nodeMap[i].name.toLowerCase();
      if ( name.indexOf("_moz") != 0 &&
           AddTreeItem(name, nodeMap[i].value, "HTMLAList", HTMLAttrs) )
      {
        added = true;
      }
    }

    if (added)
      SelectHTMLTree(0);
  }
}


function onChangeHTMLAttribute()
{
  var name = TrimString(gDialog.AddHTMLAttributeNameInput.value);
  if (!name)
    return;

  var value = TrimString(gDialog.AddHTMLAttributeValueInput.value);

  
  
  if (!UpdateExistingAttribute( name, value, "HTMLAList" ) && value)
    AddTreeItem (name, value, "HTMLAList", HTMLAttrs);
}

function ClearHTMLInputWidgets()
{
  gDialog.AddHTMLAttributeTree.view.selection.clearSelection();
  gDialog.AddHTMLAttributeNameInput.value ="";
  gDialog.AddHTMLAttributeValueInput.value = "";
  SetTextboxFocus(gDialog.AddHTMLAttributeNameInput);
}

function onSelectHTMLTreeItem()
{
  if (!gDoOnSelectTree)
    return;

  var tree = gDialog.AddHTMLAttributeTree;
  if (tree && tree.view.selection.count)
  {
    var inputName = TrimString(gDialog.AddHTMLAttributeNameInput.value).toLowerCase();
    var selectedItem = getSelectedItem(tree);
    var selectedName = selectedItem.firstChild.firstChild.getAttribute("label");

    if (inputName == selectedName)
    {
      
      gDialog.AddHTMLAttributeValueInput.value = GetTreeItemValueStr(selectedItem);
    }
    else
    {
      gDialog.AddHTMLAttributeNameInput.value = selectedName;

      
      onInputHTMLAttributeName();
    }
  }
}

function onInputHTMLAttributeName()
{
  var attName = TrimString(gDialog.AddHTMLAttributeNameInput.value).toLowerCase();

  
  gUpdateTreeValue = false;
  gDialog.AddHTMLAttributeValueInput.value = "";
  gUpdateTreeValue = true; 

  if (attName)
  {
    
    var valueListName;

    
    
    
    if (attName == "dir")
      valueListName = "all_dir";
    else
      valueListName = gElement.localName.toLowerCase() + "_" + attName;

    
    if (valueListName[0] == "_")
      valueListName = valueListName.slice(1);

    var newValue = "";
    var listLen = 0;

    
    var deckIndex = gDialog.AddHTMLAttributeValueDeck.getAttribute("selectedIndex");

    if (valueListName in gHTMLAttr)
    {
      var valueList = gHTMLAttr[valueListName];

      listLen = valueList.length;
      if (listLen > 0)
        newValue = valueList[0];

      
      
      if (listLen > 1)
      {
        gDialog.AddHTMLAttributeValueMenulist.removeAllItems();

        if (deckIndex != "1")
        {
          
          gDialog.AddHTMLAttributeValueInput = gDialog.AddHTMLAttributeValueMenulist;
          gDialog.AddHTMLAttributeValueDeck.setAttribute("selectedIndex", "1");
        }
        
        for (var i = 0; i < listLen; i++)
        {
          if (valueList[i] == "-")
          {
            
            var popup = gDialog.AddHTMLAttributeValueInput.firstChild;
            if (popup)
            {
              var sep = document.createElementNS(XUL_NS, "menuseparator");
              if (sep)
                popup.appendChild(sep);
            }        
          } else {
            gDialog.AddHTMLAttributeValueMenulist.appendItem(valueList[i]);
          }
        }
      }
    }
    
    if (listLen <= 1 && deckIndex != "0")
    {
      
      gDialog.AddHTMLAttributeValueInput = gDialog.AddHTMLAttributeValueTextbox;
      gDialog.AddHTMLAttributeValueDeck.setAttribute("selectedIndex", "0");
    }

    
    
    var existingValue = GetAndSelectExistingAttributeValue(attName, "HTMLAList");
    if (existingValue)
      newValue = existingValue;
      
    gDialog.AddHTMLAttributeValueInput.value = newValue;
  }
}

function onInputHTMLAttributeValue()
{
  if (!gUpdateTreeValue)
    return;

  var name = TrimString(gDialog.AddHTMLAttributeNameInput.value);
  if (!name)
    return;

  
  
  var value = TrimStringLeft(gDialog.AddHTMLAttributeValueInput.value);
  if (value)
  {
    
    
    
    var selectedItem = gDialog.AddHTMLAttributeNameInput.selectedItem;

    if (selectedItem)
    {
      if ( selectedItem.getAttribute("forceOneChar") == "true" &&
           value.length > 1 )
        value = value.slice(0, 1);

      if ( selectedItem.getAttribute("forceIntOrPercent") == "true" )
      {
        
        var percent = TrimStringRight(value).slice(-1);
        value = value.replace(/\D+/g,"");
        if (percent == "%")
          value += percent;
      }
      else if ( selectedItem.getAttribute("forceInteger") == "true" )
      {
        value = value.replace(/\D+/g,"");
      }
      else if ( selectedItem.getAttribute("forceSignedInteger") == "true" )
      {
        
        var sign = value[0];
        value = value.replace(/\D+/g,"");
        if (sign == "+" || sign == "-")
          value = sign + value;
      }
      
      
      if (selectedItem.getAttribute("limitFirstChar") == "true")
      {
        
        
        value = value.replace(/^[^a-zA-Z\u0080-\uFFFF]/, "").replace(/[^a-zA-Z0-9_\.\-\:\u0080-\uFFFF]+/g,'');
      }

      
      if (value != gDialog.AddHTMLAttributeValueInput.value)
        gDialog.AddHTMLAttributeValueInput.value = value;
    }
  }

  
  
  if ( !UpdateExistingAttribute(name, value, "HTMLAList" ) && value)
    AddTreeItem(name, value, "HTMLAList", HTMLAttrs);
}

function editHTMLAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    gDialog.AddHTMLAttributeValueInput.inputField.select();
}



function UpdateHTMLAttributes()
{
  var HTMLAList = document.getElementById("HTMLAList");
  var i;

  
  for (i = 0; i < HTMLRAttrs.length; i++)
  {
    var name = HTMLRAttrs[i];

    if (gElement.hasAttribute(name))
      doRemoveAttribute(name);
  }

  
  for( i = 0; i < HTMLAList.childNodes.length; i++)
  {
    var item = HTMLAList.childNodes[i];
    doSetAttribute( GetTreeItemAttributeStr(item), GetTreeItemValueStr(item));
  }
}

function RemoveHTMLAttribute()
{
  var treechildren = gDialog.AddHTMLAttributeTree.lastChild;

  
  if (gDialog.AddHTMLAttributeTree.view.selection.count)
  {
    var item = getSelectedItem(gDialog.AddHTMLAttributeTree);
    var attr = GetTreeItemAttributeStr(item);

    
    HTMLRAttrs[HTMLRAttrs.length] = attr;
    RemoveNameFromAttArray(attr, HTMLAttrs);

    
    treechildren.removeChild(item);

    
    ClearHTMLInputWidgets();
  }
}

function SelectHTMLTree( index )
{

  gDoOnSelectTree = false;
  try {
    gDialog.AddHTMLAttributeTree.selectedIndex = index;
  } catch (e) {}
  gDoOnSelectTree = true;
}
