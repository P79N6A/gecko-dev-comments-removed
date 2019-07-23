




































var qaMain = {
  htmlNS: "http://www.w3.org/1999/xhtml",

  openQATool : function() {
    window.open("chrome://qa/content/qa.xul", "_blank",
                "chrome,all,dialog=no,resizable=yes");
  },
  onToolOpen : function() {
    if (qaPref.getPref(qaPref.prefBase+'.isFirstTime', 'bool') == true) {
      window.open("chrome://qa/content/setup.xul", "_blank",
                  "chrome,all,dialog=yes");
        }
    if (qaPref.getPref(qaPref.prefBase + '.currentTestcase.testrunSummary', 'char') != null) {
            litmus.readStateFromPref();
        }
  },
    onSwitchTab : function() {
    var newSelection = $('qa_tabrow').selectedItem;

    
    if ($('qa_tabrow').selectedItem == $('qa-tabbar-prefs')) {
            qaPrefsWindow.loadPrefsWindow();
    } else if ($('qa_tabrow').selectedItem == $('qa-tabbar-bugzilla')) {
            bugzilla.unhighlightTab();
        }

    
    if (qaPrefsWindow.lastSelectedTab != null &&
        qaPrefsWindow.lastSelectedTab == $('qa-tabbar-prefs')) {
      qaPrefsWindow.savePrefsWindow();
    }

    qaPrefsWindow.lastSelectedTab = newSelection;
  }
};

qaMain.__defineGetter__("bundle", function(){return $("bundle_qa");});
qaMain.__defineGetter__("urlbundle", function(){return $("bundle_urls");});
function $() {
  var elements = new Array();

for (var i = 0; i < arguments.length; i++) {
  var element = arguments[i];
  if (typeof element == 'string')
    element = document.getElementById(element);

  if (arguments.length == 1)
    return element;

  elements.push(element);
  }

  return elements;
}
