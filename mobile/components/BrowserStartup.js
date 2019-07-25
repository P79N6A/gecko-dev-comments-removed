


































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function BrowserStartup() {
  this._init();
}
BrowserStartup.prototype = {
  
  classDescription: "Mobile Browser Glue Service",
  classID:          Components.ID("{1d542abc-c88b-4636-a4ef-075b49806317}"),
  contractID:       "@mozilla.org/mobile/browserstartup;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  
  _xpcom_categories: [
    
    { category: "app-startup", service: true }
  ],

  _xpcom_factory: BrowserStartupServiceFactory,

  _init: function () {
    this._observerService = Cc['@mozilla.org/observer-service;1'].
                            getService(Ci.nsIObserverService);
    this._observerService.addObserver(this, "places-init-complete", false);
  },

  _initDefaultBookmarks: function () {
    
    
    
    let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    
    
    let databaseStatus = histsvc.databaseStatus;
    let importBookmarks = databaseStatus == histsvc.DATABASE_STATUS_CREATE ||
                          databaseStatus == histsvc.DATABASE_STATUS_CORRUPT;

    if (!importBookmarks)
      return;

    Cu.import("resource://gre/modules/utils.js");

    
    let dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);

    let bookmarksFile = dirService.get("profDef", Ci.nsILocalFile);
    bookmarksFile.append("bookmarks.json");
    if (bookmarksFile.exists()) {
      
      try {
        PlacesUtils.restoreBookmarksFromJSONFile(bookmarksFile);
      } catch (err) {
        
        Cu.reportError("bookmarks.json file could be corrupt. " + err);
      }
    } else
      Cu.reportError("Unable to find default bookmarks.json file.");
  },

  
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "places-init-complete":
        this._initDefaultBookmarks();
        this._observerService.removeObserver(this, "places-init-complete");
        break;
    }
  }
};


const BrowserStartupServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance || (this._instance = new BrowserGlue());
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([BrowserStartup]);
