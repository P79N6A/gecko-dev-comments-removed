



































 




var EXPORTED_SYMBOLS = ["BrowserTabs"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://services-sync/engines.js");

var BrowserTabs = {
  








  Add: function(uri, fn) {
    
    
    let wm = CC["@mozilla.org/appshell/window-mediator;1"]
             .getService(CI.nsIWindowMediator);
    let mainWindow = wm.getMostRecentWindow("navigator:browser");
    let newtab = mainWindow.getBrowser().addTab(uri);
    mainWindow.getBrowser().selectedTab = newtab;
    let win = mainWindow.getBrowser().getBrowserForTab(newtab);
    win.addEventListener("load", function() { fn.call(); }, true);
  },

  










  Find: function(uri, title, profile) {
    
    let engine = Engines.get("tabs");
    for (let [guid, client] in Iterator(engine.getAllClients())) {
      for each (tab in client.tabs) {
        let weaveTabUrl = tab.urlHistory[0];
        if (uri == weaveTabUrl && profile == client.clientName)
          if (title == undefined || title == tab.title)
            return true;
      }
    }
    return false;
  },
};

