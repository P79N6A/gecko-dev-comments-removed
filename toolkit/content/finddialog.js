





Components.utils.import("resource://gre/modules/Services.jsm");

var dialog;     
var gFindInst;   
var gFindInstData; 

function initDialogObject()
{
  
  dialog = new Object;
  dialog.findKey         = document.getElementById("dialog.findKey");
  dialog.caseSensitive   = document.getElementById("dialog.caseSensitive");
  dialog.wrap            = document.getElementById("dialog.wrap");
  dialog.find            = document.getElementById("btnFind");
  dialog.up              = document.getElementById("radioUp");
  dialog.down            = document.getElementById("radioDown");
  dialog.rg              = dialog.up.radioGroup;
  dialog.bundle          = null;

  
  var windowElement = document.getElementById("findDialog");
  if (!windowElement.hasAttribute("screenX") || !windowElement.hasAttribute("screenY"))
  {
    sizeToContent();
    moveToAlertPosition();
  }
}

function fillDialog()
{
  
  var findService = Components.classes["@mozilla.org/find/find_service;1"]
                              .getService(Components.interfaces.nsIFindService);
  
  
  
  dialog.findKey.value           = gFindInst.searchString ? gFindInst.searchString : findService.searchString;
  dialog.caseSensitive.checked   = gFindInst.matchCase ? gFindInst.matchCase : findService.matchCase;
  dialog.wrap.checked            = gFindInst.wrapFind ? gFindInst.wrapFind : findService.wrapFind;
  var findBackwards              = gFindInst.findBackwards ? gFindInst.findBackwards : findService.findBackwards;
  if (findBackwards)
    dialog.rg.selectedItem = dialog.up;
  else
    dialog.rg.selectedItem = dialog.down;
}

function saveFindData()
{
  
  var findService = Components.classes["@mozilla.org/find/find_service;1"]
                         .getService(Components.interfaces.nsIFindService);

  
  findService.searchString  = dialog.findKey.value;
  findService.matchCase     = dialog.caseSensitive.checked;
  findService.wrapFind      = dialog.wrap.checked;
  findService.findBackwards = dialog.up.selected;
}

function onLoad()
{
  initDialogObject();

  
  var arg0 = window.arguments[0];                                               
  
  
  if (arg0 instanceof Components.interfaces.nsIWebBrowserFind) {
    gFindInst = arg0;
  } else {  
    gFindInstData = arg0;                                                       
    gFindInst = gFindInstData.webBrowserFind;                                   
  }                                                                             

  fillDialog();
  doEnabling();

  if (dialog.findKey.value)
    dialog.findKey.select();
  dialog.findKey.focus();
}

function onUnload()
{
  window.opener.findDialog = 0;
}

function onAccept()
{
  if (gFindInstData && gFindInst != gFindInstData.webBrowserFind) {
    gFindInstData.init();             
    gFindInst = gFindInstData.webBrowserFind;
  }

  
  saveFindData();

  
  gFindInst.searchString  = dialog.findKey.value;
  gFindInst.matchCase     = dialog.caseSensitive.checked;
  gFindInst.wrapFind      = dialog.wrap.checked;
  gFindInst.findBackwards = dialog.up.selected;
  
  
  var result = gFindInst.findNext();

  if (!result)
  {
    if (!dialog.bundle)
      dialog.bundle = document.getElementById("findBundle");
    Services.prompt.alert(window, dialog.bundle.getString("notFoundTitle"),
                                  dialog.bundle.getString("notFoundWarning"));
    dialog.findKey.select();
    dialog.findKey.focus();
  } 
  return false;
}

function doEnabling()
{
  dialog.find.disabled = !dialog.findKey.value;
}
