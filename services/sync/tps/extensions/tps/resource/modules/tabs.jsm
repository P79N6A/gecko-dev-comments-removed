



 




const EXPORTED_SYMBOLS = ["BrowserTabs"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-sync/main.js");

let BrowserTabs = {
  








  Add: function(uri, fn) {
    
    
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
               .getService(Ci.nsIWindowMediator);
    let mainWindow = wm.getMostRecentWindow("navigator:browser");
    let newtab = mainWindow.getBrowser().addTab(uri);
    mainWindow.getBrowser().selectedTab = newtab;
    let win = mainWindow.getBrowser().getBrowserForTab(newtab);
    win.addEventListener("load", function() { fn.call(); }, true);
  },

  










  Find: function(uri, title, profile) {
    
    let engine = Weave.Service.engineManager.get("tabs");
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

