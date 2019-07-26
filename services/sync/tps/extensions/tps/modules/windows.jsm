


"use strict";

 




const EXPORTED_SYMBOLS = ["BrowserWindows"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-sync/main.js");

let BrowserWindows = {
  







  Add: function(aPrivate, fn) {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
               .getService(Ci.nsIWindowMediator);
    let mainWindow = wm.getMostRecentWindow("navigator:browser");
    let win = mainWindow.OpenBrowserWindow({private: aPrivate});
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      fn.call(win);
    }, false);
  }
};
