



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const DOWNLOAD_MANAGER_URL = "chrome://mozapps/content/downloads/downloads.xul";
const PREF_FLASH_COUNT = "browser.download.manager.flashCount";




function nsDownloadManagerUI() {}

nsDownloadManagerUI.prototype = {
  classDescription: "Used to show the Download Manager's UI to the user",
  classID: Components.ID("7dfdf0d1-aff6-4a34-bad1-d0fe74601642"),
  contractID: "@mozilla.org/download-manager-ui;1",

  
  

  show: function show(aWindowContext, aID)
  {
    
    if (this.recentWindow) {
      this.recentWindow.focus();
      return;
    }

    
    
    
    var window = null;
    try {
      if (aWindowContext)
        window = aWindowContext.getInterface(Ci.nsIDOMWindow);
    } catch (e) {  }

    
    var params = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    var dm = Cc["@mozilla.org/download-manager;1"].
             getService(Ci.nsIDownloadManager);
    params.appendElement(dm, false);

    
    var download = null;
    try {
      download = dm.getDownload(aID);
    } catch (ex) {}
    params.appendElement(download, false);

    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    ww.openWindow(window,
                  DOWNLOAD_MANAGER_URL,
                  "Download:Manager",
                  "chrome,dialog=no,resizable",
                  params);
  },

  get visible() {
    return (null != this.recentWindow);
  },

  getAttention: function getAttention()
  {
    if (!this.visible)
      throw Cr.NS_ERROR_UNEXPECTED;

    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    
    let flashCount = 2;
    try {
      flashCount = prefs.getIntPref(PREF_FLASH_COUNT);
    } catch (e) { }

    var win = this.recentWindow.QueryInterface(Ci.nsIDOMChromeWindow);
    win.getAttentionWithCycleCount(flashCount);
  },

  
  

  get recentWindow() {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    return wm.getMostRecentWindow("Download:Manager");
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadManagerUI])
};




let components = [nsDownloadManagerUI];

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}

