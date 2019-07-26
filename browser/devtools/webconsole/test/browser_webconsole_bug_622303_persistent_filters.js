


let prefService = Services.prefs;

let prefs = {
  "net": [
    "network",
    "networkinfo"
  ],
  "css": [
    "csserror",
    "cssparser"
  ],
  "js": [
    "exception",
    "jswarn"
  ],
  "logging": [
     "error",
     "warn",
     "info",
     "log"
  ]
};

function test() {
  
  for (let category in prefs) {
    prefs[category].forEach(function(pref) {
      prefService.setBoolPref("devtools.webconsole.filter." + pref, true);
    });
  }

  addTab("about:blank");
  openConsole(null, onConsoleOpen);
}

function onConsoleOpen(hud) {
  let hudBox = hud.ui.rootElement;

  
  for (let category in prefs) {
    let button = hudBox.querySelector(".webconsole-filter-button[category=\""
                                      + category + "\"]");
    ok(isChecked(button), "main button for " + category + " category is checked");

    prefs[category].forEach(function(pref) {
      let menuitem = hudBox.querySelector("menuitem[prefKey=" + pref + "]");
      ok(isChecked(menuitem), "menuitem for " + pref + " is checked");
    });
  }

  
  for (let category in prefs) {
    prefs[category].forEach(function(pref) {
      hud.setFilterState(pref, false);
    });
  }

  
  closeConsole(null, function() {
    openConsole(null, onConsoleReopen1);
  });
}

function onConsoleReopen1(hud) {
  let hudBox = hud.ui.rootElement;

  
  for (let category in prefs) {
    let button = hudBox.querySelector(".webconsole-filter-button[category=\""
                                           + category + "\"]");
    ok(isUnchecked(button), "main button for " + category + " category is not checked");

    prefs[category].forEach(function(pref) {
      let menuitem = hudBox.querySelector("menuitem[prefKey=" + pref + "]");
      ok(isUnchecked(menuitem), "menuitem for " + pref + " is not checked");
    });
  }

  
  for (let category in prefs) {
    hud.setFilterState(prefs[category][0], true);
  }

  
  closeConsole(null, function() {
    openConsole(null, onConsoleReopen2);
  });
}

function onConsoleReopen2(hud) {
  let hudBox = hud.ui.rootElement;

  
  for (let category in prefs) {
    let button = hudBox.querySelector(".webconsole-filter-button[category=\""
                                           + category + "\"]");
    ok(isChecked(button), category  + " button is checked when first pref is true");

    let pref = prefs[category][0];
    let menuitem = hudBox.querySelector("menuitem[prefKey=" + pref + "]");
    ok(isChecked(menuitem), "first " + category + " menuitem is checked");
  }

  
  for (let category in prefs) {
    prefs[category].forEach(function(pref) {
      prefService.clearUserPref("devtools.webconsole.filter." + pref);
    });
  }

  prefs = prefService = null;
  gBrowser.removeCurrentTab();
  finishTest();
}

function isChecked(aNode) {
  return aNode.getAttribute("checked") === "true";
}

function isUnchecked(aNode) {
  return aNode.getAttribute("checked") === "false";
}
