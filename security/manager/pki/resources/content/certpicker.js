




const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;

var dialogParams;
var itemCount = 0;

function onLoad()
{
  dialogParams = window.arguments[0].QueryInterface(nsIDialogParamBlock);

  var selectElement = document.getElementById("nicknames");
  itemCount = dialogParams.GetInt(0);

  var selIndex = dialogParams.GetInt(1);
  if (selIndex < 0)
    selIndex = 0;
  
  for (var i=0; i < itemCount; i++) {
      var menuItemNode = document.createElement("menuitem");
      var nick = dialogParams.GetString(i);
      menuItemNode.setAttribute("value", i);
      menuItemNode.setAttribute("label", nick); 
      selectElement.firstChild.appendChild(menuItemNode);
      
      if (selIndex == i) {
        selectElement.selectedItem = menuItemNode;
      }
  }

  dialogParams.SetInt(0,0); 
  setDetails();
}

function setDetails()
{
  var selItem = document.getElementById("nicknames").value;
  if (!selItem.length)
    return;

  var index = parseInt(selItem);
  var details = dialogParams.GetString(index+itemCount);
  document.getElementById("details").value = details;
}

function onCertSelected()
{
  setDetails();
}

function doOK()
{
  dialogParams.SetInt(0,1);
  var index = parseInt(document.getElementById("nicknames").value);
  dialogParams.SetInt(1, index);
  return true;
}

function doCancel()
{
  dialogParams.SetInt(0,0);
  return true;
}
