








































var browser;
var dialog = {};
var gNavigatorBundle;
var pref = null;
try {
  pref = Components.classes["@mozilla.org/preferences-service;1"]
                   .getService(Components.interfaces.nsIPrefBranch);
} catch (ex) {
  
}

function onLoad()
{
  dialog.input          = document.getElementById("dialog.input");
  dialog.open           = document.documentElement.getButton("accept");
  dialog.openAppList    = document.getElementById("openAppList");
  dialog.openTopWindow  = document.getElementById("currentWindow");
  dialog.openEditWindow = document.getElementById("editWindow");
  dialog.bundle         = document.getElementById("openLocationBundle");
  gNavigatorBundle      = document.getElementById("navigatorBundle");

  if ("arguments" in window && window.arguments.length >= 1)
    browser = window.arguments[0];
   
  if (!browser) {
    
    dialog.openAppList.selectedItem = dialog.openEditWindow;

    
    dialog.openTopWindow.setAttribute("label", dialog.bundle.getString("existingNavigatorWindow"));

    
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    if (windowManagerInterface)
      browser = windowManagerInterface.getMostRecentWindow( "navigator:browser" );

    
    if (!browser)
      dialog.openTopWindow.setAttribute("disabled", "true");
  }
  else {
    dialog.openAppList.selectedItem = dialog.openTopWindow;
  }

  if (pref) {
    try {
      var value = pref.getIntPref("general.open_location.last_window_choice");
      var element = dialog.openAppList.getElementsByAttribute("value", value)[0];
      if (element)
        dialog.openAppList.selectedItem = element;
      dialog.input.value = pref.getComplexValue("general.open_location.last_url",
                                                Components.interfaces.nsISupportsString).data;
    }
    catch(ex) {
    }
    if (dialog.input.value)
      dialog.input.select(); 
  }

  doEnabling();
}

function doEnabling()
{
    dialog.open.disabled = !dialog.input.value;
}

function open()
{
  var url;
  if (browser)
    url = browser.getShortcutOrURI(dialog.input.value);
  else
    url = dialog.input.value;

  try {
    switch (dialog.openAppList.value) {
      case "0":
        browser.loadURI(url);
        break;
      case "1":
        window.opener.delayedOpenWindow(getBrowserURL(), "all,dialog=no", url);
        break;
      case "2":
        
        
        if ("editPage" in window.opener)
          window.opener.editPage(url, window.opener, true);
        break;
      case "3":
        if (browser.getBrowser && browser.getBrowser().localName == "tabbrowser")
          browser.delayedOpenTab(url);
        else
          browser.loadURI(url); 
        break;
    }
  }
  catch(exception) {
  }

  if (pref) {
    var str = Components.classes["@mozilla.org/supports-string;1"]
                        .createInstance(Components.interfaces.nsISupportsString);
    str.data = dialog.input.value;
    pref.setComplexValue("general.open_location.last_url",
                         Components.interfaces.nsISupportsString, str);
    pref.setIntPref("general.open_location.last_window_choice", dialog.openAppList.value);
  }

  
  window.close();
  return false;
}

function createInstance(contractid, iidName)
{
  var iid = Components.interfaces[iidName];
  return Components.classes[contractid].createInstance(iid);
}

const nsIFilePicker = Components.interfaces.nsIFilePicker;
function onChooseFile()
{
  try {
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    fp.init(window, dialog.bundle.getString("chooseFileDialogTitle"), nsIFilePicker.modeOpen);
    if (dialog.openAppList.value == "2") {
      
      
      fp.appendFilters(nsIFilePicker.filterHTML | nsIFilePicker.filterText);
      fp.appendFilters(nsIFilePicker.filterText);
      fp.appendFilters(nsIFilePicker.filterAll);
    }
    else {
      fp.appendFilters(nsIFilePicker.filterHTML | nsIFilePicker.filterText |
                       nsIFilePicker.filterAll | nsIFilePicker.filterImages | nsIFilePicker.filterXML);
    }

    if (fp.show() == nsIFilePicker.returnOK && fp.fileURL.spec && fp.fileURL.spec.length > 0)
      dialog.input.value = fp.fileURL.spec;
  }
  catch(ex) {
  }
  doEnabling();
}

function useUBHistoryItem(aMenuItem)
{
  var urlbar = document.getElementById("dialog.input");
  urlbar.value = aMenuItem.getAttribute("label");
  urlbar.focus();
  doEnabling();
}

