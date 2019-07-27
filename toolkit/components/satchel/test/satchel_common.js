



var Services = SpecialPowers.Services;
var gPopupShownListener;
var gLastAutoCompleteResults;






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
    var key = SpecialPowers.Ci.nsIDOMKeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = SpecialPowers.getDOMWindowUtils(window);

    if (wutils.sendKeyEvent("keydown",  key, 0, modifier)) {
      wutils.sendKeyEvent("keypress", key, 0, modifier);
    }
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}

function registerPopupShownListener(listener) {
  if (gPopupShownListener) {
    ok(false, "got too many popupshownlisteners");
    return;
  }
  gPopupShownListener = listener;
}

function getMenuEntries() {
  if (!gLastAutoCompleteResults) {
    throw new Error("no autocomplete results");
  }

  var results = gLastAutoCompleteResults;
  gLastAutoCompleteResults = null;
  return results;
}

var checkObserver = {
  verifyStack: [],
  callback: null,

  init() {
    script.sendAsyncMessage("addObserver");
    script.addMessageListener("satchel-storage-changed", this.observe.bind(this));
  },

  uninit() {
    script.sendAsyncMessage("removeObserver");
  },

  waitForChecks: function(callback) {
    if (this.verifyStack.length == 0)
      callback();
    else
      this.callback = callback;
  },

  observe: function({ subject, topic, data }) {
    if (data != "formhistory-add" && data != "formhistory-update")
      return;
    ok(this.verifyStack.length > 0, "checking if saved form data was expected");

    
    
    
    
    
    
    
    
    
    var expected = this.verifyStack.shift();

    countEntries(expected.name, expected.value,
      function(num) {
        ok(num > 0, expected.message);
        if (checkObserver.verifyStack.length == 0) {
          var callback = checkObserver.callback;
          checkObserver.callback = null;
          callback();
        }
      });
  }
};

function checkForSave(name, value, message) {
  checkObserver.verifyStack.push({ name : name, value: value, message: message });
}

function NonE10SgetAutocompletePopup() {
  var Ci = SpecialPowers.Ci;
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


function getFormSubmitButton(formNum) {
  var form = $("form" + formNum); 
  ok(form != null, "getting form " + formNum);

  
  
  var button = form.firstChild;
  while (button && button.type != "submit") { button = button.nextSibling; }
  ok(button != null, "getting form submit button");

  return button;
}



function countEntries(name, value, then) {
  script.sendAsyncMessage("countEntries", { name, value });
  script.addMessageListener("entriesCounted", function counted(data) {
    script.removeMessageListener("entriesCounted", counted);
    if (!data.ok) {
      ok(false, "Error occurred counting form history");
      SimpleTest.finish();
      return;
    }

    then(data.count);
  });
}


function updateFormHistory(changes, then) {
  script.sendAsyncMessage("updateFormHistory", { changes });
  script.addMessageListener("formHistoryUpdated", function updated({ ok }) {
    script.removeMessageListener("formHistoryUpdated", updated);
    if (!ok) {
      ok(false, "Error occurred updating form history");
      SimpleTest.finish();
      return;
    }

    then();
  });
}

function notifyMenuChanged(expectedCount, expectedFirstValue, then) {
  script.sendAsyncMessage("waitForMenuChange",
                          { expectedCount,
                            expectedFirstValue });
  script.addMessageListener("gotMenuChange", function changed({ results }) {
    script.removeMessageListener("gotMenuChange", changed);
    gLastAutoCompleteResults = results;
    then();
  });
}

var chromeURL = SimpleTest.getTestFileURL("parent_utils.js");
var script = SpecialPowers.loadChromeScript(chromeURL);
script.addMessageListener("onpopupshown", ({ results }) => {
  gLastAutoCompleteResults = results;
  if (gPopupShownListener)
    gPopupShownListener();
});

SimpleTest.registerCleanupFunction(() => {
  script.sendAsyncMessage("cleanup");
  script.destroy();
});
