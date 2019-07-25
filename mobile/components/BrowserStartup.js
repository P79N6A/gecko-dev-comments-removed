


































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


const BrowserStartupServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance || (this._instance = new BrowserStartup());
  }
};

function BrowserStartup() {
  this._init();
}

BrowserStartup.prototype = {
  
  classDescription: "Mobile Browser Glue Service",
  classID:          Components.ID("{1d542abc-c88b-4636-a4ef-075b49806317}"),
  contractID:       "@mozilla.org/mobile/browser-startup;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  
  _xpcom_categories: [
    
    { category: "app-startup", service: true }
  ],

  _xpcom_factory: BrowserStartupServiceFactory,

  _init: function () {
    this._observerService = Cc["@mozilla.org/observer-service;1"].
                            getService(Ci.nsIObserverService);
    this._observerService.addObserver(this, "places-init-complete", false);
  },

  _initDefaultBookmarks: function () {
    
    
    
    let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    
    
    let databaseStatus = histsvc.databaseStatus;
    let importBookmarks = databaseStatus == histsvc.DATABASE_STATUS_CREATE ||
                          databaseStatus == histsvc.DATABASE_STATUS_CORRUPT;

    if (!importBookmarks) {
      
      
      
      
      let annos = Cc["@mozilla.org/browser/annotation-service;1"].
                  getService(Ci.nsIAnnotationService);
      let mobileRootItems = annos.getItemsWithAnnotation("mobile/bookmarksRoot", {});
      if (mobileRootItems.length > 0)
        return; 
    }

    Cu.import("resource://gre/modules/utils.js");

    try {
      let observer = {
        onStreamComplete : function(aLoader, aContext, aStatus, aLength, aResult) {
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                          createInstance(Ci.nsIScriptableUnicodeConverter);
          let jsonStr = "";
          try {
            converter.charset = "UTF-8";
            jsonStr = converter.convertFromByteArray(aResult, aResult.length);

            
            
            
            PlacesUtils.restoreBookmarksFromJSONString(jsonStr, false);
          } catch (err) {
            Cu.reportError("Failed to parse default bookmarks from bookmarks.json: " + err);
          }
        }
      };

      let ioSvc = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
      let uri = ioSvc.newURI("chrome://browser/locale/bookmarks.json", null, null);
      let channel = ioSvc.newChannelFromURI(uri);
      let sl = Cc["@mozilla.org/network/stream-loader;1"].
               createInstance(Ci.nsIStreamLoader);
      sl.init(observer);
      channel.asyncOpen(sl, channel);
    } catch (err) {
      
      Cu.reportError("Failed to load default bookmarks from bookmarks.json: " + err);
    }
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


function BrowserStyleSheets() { }

BrowserStyleSheets.prototype = {
  
  
  classDescription: "Mobile Browser Content Stylesheets",
  classID:          Components.ID("9e899d1c-dd83-44e9-ab01-3180b2c5b2b1"),
  contractID:       "@mozilla.org/mobile/browser-stylesheets;1",
  QueryInterface: XPCOMUtils.generateQI([]),

  _xpcom_categories: [{ category: "agent-style-sheets",
                        entry:    "browser content stylesheet",
                        value:    "chrome://browser/content/content.css"},
                      { category: "agent-style-sheets",
                        entry:    "browser cursor stylesheet",
                        value:    "chrome://browser/content/cursor.css"}]
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([BrowserStartup, BrowserStyleSheets]);
