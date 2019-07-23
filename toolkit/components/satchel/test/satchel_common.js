



































 





function $_(formNum, name) {
  var form = document.getElementById("form" + formNum);
  if (!form) {
    ok(false, "$_ couldn't find requested form " + formNum);
    return null;
  }

  var element = form.elements.namedItem(name);
  if (!element) {
    ok(false, "$_ couldn't find requested element " + name);
    return null;
  }

  
  
  

  if (element.hasAttribute("name") && element.getAttribute("name") != name) {
    ok(false, "$_ got confused.");
    return null;
  }

  return element;
}



function doKey(aKey, modifier) {
    
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

    var keyName = "DOM_VK_" + aKey.toUpperCase();
    var key = Components.interfaces.nsIDOMKeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor).
                          getInterface(Components.interfaces.nsIDOMWindowUtils);

    wutils.sendKeyEvent("keydown",  key, 0, modifier);
    wutils.sendKeyEvent("keypress", key, 0, modifier);
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}


function getAutocompletePopup() {
    var Ci = Components.interfaces;
    chromeWin = window
                    .QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIWebNavigation)
                    .QueryInterface(Ci.nsIDocShellTreeItem)
                    .rootTreeItem
                    .QueryInterface(Ci.nsIInterfaceRequestor)
                    .getInterface(Ci.nsIDOMWindow)
                    .QueryInterface(Ci.nsIDOMChromeWindow);
    autocompleteMenu = chromeWin.document.getElementById("PopupAutoComplete");
    ok(autocompleteMenu, "Got autocomplete popup");

    return autocompleteMenu;
}


function cleanUpFormHist() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var formhist = Components.classes["@mozilla.org/satchel/form-history;1"].
                 getService(Components.interfaces.nsIFormHistory2);
  formhist.removeAllEntries();
}
cleanUpFormHist();
