



"use strict";

Components.utils.import("resource://gre/modules/Services.jsm");

if (!Services.prefs.getBoolPref("browser.search.showOneOffButtons")) {
  addEventListener("load", function onLoad() {
    removeEventListener("load", onLoad);
    let pane =
      document.getAnonymousElementByAttribute(document.documentElement,
                                              "pane", "paneSearch");
    pane.hidden = true;
    if (pane.selected)
      document.documentElement.showPane(document.getElementById("paneMain"));
  });
}
