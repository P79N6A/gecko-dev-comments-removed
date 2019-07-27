



"use strict";

Components.utils.import("resource://gre/modules/Services.jsm");

addEventListener("load", function () {
  
  
  
  if (Services.prefs.getBoolPref("browser.readinglist.enabled")) {
    document.getElementById("readinglist-engine").removeAttribute("hidden");
  }
});

addEventListener("dialogaccept", function () {
  let pane = document.getElementById("sync-customize-pane");
  
  
  let prefElts = pane.querySelectorAll("preferences > preference");
  let syncEnabled = false;
  for (let elt of prefElts) {
    if (elt.name.startsWith("services.sync.") && elt.value) {
      syncEnabled = true;
      break;
    }
  }
  Services.prefs.setBoolPref("services.sync.enabled", syncEnabled);
  
  pane.writePreferences(true);
  window.arguments[0].accepted = true;
});
