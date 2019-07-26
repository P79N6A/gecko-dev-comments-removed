



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
    var key = SpecialPowers.Ci.nsIDOMKeyEvent[keyName];

    
    if (!modifier)
        modifier = null;

    
    var wutils = SpecialPowers.getDOMWindowUtils(window);

    if (wutils.sendKeyEvent("keydown",  key, 0, modifier)) {
      wutils.sendKeyEvent("keypress", key, 0, modifier);
    }
    wutils.sendKeyEvent("keyup",    key, 0, modifier);
}


function getAutocompletePopup() {
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


function cleanUpFormHist() {
  SpecialPowers.formHistory.update({ op : "remove" });
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


function getFormSubmitButton(formNum) {
  var form = $("form" + formNum); 
  ok(form != null, "getting form " + formNum);

  
  
  var button = form.firstChild;
  while (button && button.type != "submit") { button = button.nextSibling; }
  ok(button != null, "getting form submit button");

  return button;
}



function countEntries(name, value, then) {
  var obj = {};
  if (name !== null)
    obj.fieldname = name;
  if (value !== null)
    obj.value = value;

  var count = 0;
  SpecialPowers.formHistory.count(obj, SpecialPowers.wrapCallbackObject({ handleResult: function (result) { count = result },
                                         handleError: function (error) {
                                           ok(false, "Error occurred searching form history: " + error.message);
                                           SimpleTest.finish();
                                         },
                                         handleCompletion: function (reason) { if (!reason) then(count); }
                                       }));
}


function updateFormHistory(changes, then) {
  SpecialPowers.formHistory.update(changes, SpecialPowers.wrapCallbackObject({ handleError: function (error) {
                                                ok(false, "Error occurred updating form history: " + error.message);
                                                SimpleTest.finish();
                                              },
                                              handleCompletion: function (reason) { if (!reason) then(); },
                                            }));
}
