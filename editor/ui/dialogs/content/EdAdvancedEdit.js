








































var gElement    = null; 

var HTMLAttrs   = [];   
var CSSAttrs    = [];   
var JSEAttrs    = [];   

var HTMLRAttrs  = [];   
var JSERAttrs   = [];   




var gDoOnSelectTree = true;
var gUpdateTreeValue = true;









function Startup()
{
  var editor = GetCurrentEditor();

  
  if (!editor || !window.arguments[1])
  {
    dump("Advanced Edit: No editor or element to edit not supplied\n");
    window.close();
    return;
  }
  
  
  window.opener.AdvancedEditOK = false;

  
  gElement = window.arguments[1];

  
  var tagLabel = document.getElementById("tagLabel");
  tagLabel.setAttribute("value", ("<" + gElement.localName + ">"));

  
  gDialog.AddHTMLAttributeNameInput  = document.getElementById("AddHTMLAttributeNameInput");

  
  gDialog.AddHTMLAttributeValueDeck     = document.getElementById("AddHTMLAttributeValueDeck");
  gDialog.AddHTMLAttributeValueMenulist = document.getElementById("AddHTMLAttributeValueMenulist");
  gDialog.AddHTMLAttributeValueTextbox  = document.getElementById("AddHTMLAttributeValueTextbox");
  gDialog.AddHTMLAttributeValueInput    = gDialog.AddHTMLAttributeValueTextbox;

  gDialog.AddHTMLAttributeTree          = document.getElementById("HTMLATree");
  gDialog.AddCSSAttributeNameInput      = document.getElementById("AddCSSAttributeNameInput");
  gDialog.AddCSSAttributeValueInput     = document.getElementById("AddCSSAttributeValueInput");
  gDialog.AddCSSAttributeTree           = document.getElementById("CSSATree");
  gDialog.AddJSEAttributeNameList       = document.getElementById("AddJSEAttributeNameList");
  gDialog.AddJSEAttributeValueInput     = document.getElementById("AddJSEAttributeValueInput");
  gDialog.AddJSEAttributeTree           = document.getElementById("JSEATree");
  gDialog.okButton                      = document.documentElement.getButton("accept");

  
  BuildHTMLAttributeTable();
  BuildCSSAttributeTable();
  BuildJSEAttributeTable();
  
  
  BuildJSEAttributeNameList();
  BuildHTMLAttributeNameList();
  

  
  SetTextboxFocus(gDialog.AddHTMLAttributeNameInput);

  
  window.sizeToContent();

  SetWindowLocation();
}







function onAccept()
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();
  try {
    
    UpdateHTMLAttributes();
    UpdateCSSAttributes();
    UpdateJSEAttributes();
  } catch(ex) {
    dump(ex);
  }
  editor.endTransaction();

  window.opener.AdvancedEditOK = true;
  SaveWindowLocation();

  return true; 
}




function doRemoveAttribute(attrib)
{
  try {
    var editor = GetCurrentEditor();
    if (gElement.parentNode)
      editor.removeAttribute(gElement, attrib);
    else
      gElement.removeAttribute(attrib);
  } catch(ex) {}
}

function doSetAttribute(attrib, value)
{
  try {
    var editor = GetCurrentEditor();
    if (gElement.parentNode)
      editor.setAttribute(gElement, attrib, value);
    else
      gElement.setAttribute(attrib, value);
  } catch(ex) {}
}








function CheckAttributeNameSimilarity(attName, attArray)
{
  for (var i = 0; i < attArray.length; i++)
  {
    if (attName.toLowerCase() == attArray[i].toLowerCase())
      return true;
  }
  return false;
}








function UpdateExistingAttribute( attName, attValue, treeChildrenId )
{
  var treeChildren = document.getElementById(treeChildrenId);
  if (!treeChildren)
    return false;

  var name;
  var i;
  attName = TrimString(attName).toLowerCase();
  attValue = TrimString(attValue);

  for (i = 0; i < treeChildren.childNodes.length; i++)
  {
    var item = treeChildren.childNodes[i];
    name = GetTreeItemAttributeStr(item);
    if (name.toLowerCase() == attName)
    {
      
      SetTreeItemValueStr(item, attValue);

      
      
      gDoOnSelectTree = false;
      try {
        selectTreeItem(treeChildren, item);
      } catch (e) {}
      gDoOnSelectTree = true;

      return true;
    }
  }
  return false;
}






function GetAndSelectExistingAttributeValue( attName, treeChildrenId )
{
  if (!attName)
    return "";

  var treeChildren = document.getElementById(treeChildrenId);
  var name;
  var i;

  for (i = 0; i < treeChildren.childNodes.length; i++)
  {
    var item = treeChildren.childNodes[i];
    name = GetTreeItemAttributeStr(item);
    if (name.toLowerCase() == attName.toLowerCase())
    {
      
      
      gDoOnSelectTree = false;
      try {
        selectTreeItem(treeChildren, item);
      } catch (e) {}
      gDoOnSelectTree = true;

      
      return GetTreeItemValueStr(item);
    }
  }

  
  gDoOnSelectTree = false;
  try {
    treeChildren.parentNode.view.selection.clearSelection();
  } catch (e) {}
  gDoOnSelectTree = true;

  return "";
}







function GetTreeItemAttributeStr(treeItem)
{
  if (treeItem)
    return TrimString(treeItem.firstChild.firstChild.getAttribute("label"));

  return "";
}

function GetTreeItemValueStr(treeItem)
{
  if (treeItem)
    return TrimString(treeItem.firstChild.lastChild.getAttribute("label"));

  return "";
}

function SetTreeItemValueStr(treeItem, value)
{
  if (treeItem && GetTreeItemValueStr(treeItem) != value)
    treeItem.firstChild.lastChild.setAttribute("label", value);
}

function IsNotTreeHeader(treeCell)
{
  if (treeCell)
    return (treeCell.parentNode.parentNode.nodeName != "treehead");

  return false;
}

function RemoveNameFromAttArray(attName, attArray)
{
  for (var i=0; i < attArray.length; i++)
  {
    if (attName.toLowerCase() == attArray[i].toLowerCase())
    {
      
      attArray.splice(i,1);
      break;
    }
  }
}


function AddTreeItem ( name, value, treeChildrenId, attArray )
{
  attArray[attArray.length] = name;
  var treeChildren    = document.getElementById ( treeChildrenId );
  var treeitem    = document.createElementNS ( XUL_NS, "treeitem" );
  var treerow     = document.createElementNS ( XUL_NS, "treerow" );

  var attrCell    = document.createElementNS ( XUL_NS, "treecell" );
  attrCell.setAttribute( "class", "propertylist" );
  attrCell.setAttribute( "label", name );

  var valueCell    = document.createElementNS ( XUL_NS, "treecell" );
  valueCell.setAttribute( "class", "propertylist" );
  valueCell.setAttribute( "label", value );

  treerow.appendChild ( attrCell );
  treerow.appendChild ( valueCell );
  treeitem.appendChild ( treerow );
  treeChildren.appendChild ( treeitem );

  
  
  gDoOnSelectTree = false;
  try {
    selectTreeItem(treeChildren, treeitem);
  } catch (e) {}
  gDoOnSelectTree = true;

  return treeitem;
}

function selectTreeItem(treeChildren, item)
{
  var index = treeChildren.parentNode.contentView.getIndexOfItem(item);
  treeChildren.parentNode.view.selection.select(index);
}

function getSelectedItem(tree)
{
  if (tree.view.selection.count == 1)
    return tree.contentView.getItemAtIndex(tree.currentIndex);
  else
    return null;
}
