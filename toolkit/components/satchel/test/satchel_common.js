



var Services = SpecialPowers.Services;






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
    var keyName = "DOM_VK_" + aKey.toUpperCase();
    var key = Components.interfaces.nsIDOMKeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = SpecialPowers.getDOMWindowUtils(window);

    wutils.sendKeyEvent("keydown",  key, 0, modifier);
    wutils.sendKeyEvent("keypress", key, 0, modifier);
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}


function getAutocompletePopup() {
    var Ci = Components.interfaces;
    chromeWin = SpecialPowers.wrap(window)
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
  var formhist = SpecialPowers.wrap(Components).classes["@mozilla.org/satchel/form-history;1"].
                 getService(Components.interfaces.nsIFormHistory2);
  formhist.removeAllEntries();
}
cleanUpFormHist();


var checkObserver = {
  verifyStack: [],
  callback: null,

  waitForChecks: function(callback) {
    if (this.verifyStack.length == 0)
      callback();
    else
      this.callback = callback;
  },

  observe: function(subject, topic, data) {
    if (data != "addEntry" && data != "modifyEntry")
      return;
    ok(this.verifyStack.length > 0, "checking if saved form data was expected");

    
    
    
    
    
    
    
    
    
    var expected = this.verifyStack.shift();
    ok(fh.entryExists(expected.name, expected.value), expected.message);

    if (this.verifyStack.length == 0) {
      var callback = this.callback;
      this.callback = null;
      callback();
    }
  }
};

function checkForSave(name, value, message) {
  checkObserver.verifyStack.push({ name : name, value: value, message: message });
}


function getFormSubmitButton(formNum) {
  var form = $("form" + formNum); 
  ok(form != null, "getting form " + formNum);

  
  
  var button = form.firstChild;
  while (button && button.type != "submit") { button = button.nextSibling; }
  ok(button != null, "getting form submit button");

  return button;
}
