


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


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
  
  classID: Components.ID("{1d542abc-c88b-4636-a4ef-075b49806317}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  _xpcom_factory: BrowserStartupServiceFactory,

  _init: function() {
    Services.obs.addObserver(this, "places-init-complete", false);
    Services.obs.addObserver(this, "final-ui-startup", false);
  },

  _initDefaultBookmarks: function() {
    
    
    
    let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    
    
    let databaseStatus = histsvc.databaseStatus;
    let importBookmarks = databaseStatus == histsvc.DATABASE_STATUS_CREATE ||
                          databaseStatus == histsvc.DATABASE_STATUS_CORRUPT;

    if (!importBookmarks) {
      
      
      
      
      let annos = Cc["@mozilla.org/browser/annotation-service;1"].
                  getService(Ci.nsIAnnotationService);
      let metroRootItems = annos.getItemsWithAnnotation("metro/bookmarksRoot", {});
      if (metroRootItems.length > 0)
        return; 
    }

    Cu.import("resource://gre/modules/PlacesUtils.jsm");

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

  _startupActions: function() {
  },

  
  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "places-init-complete":
        Services.obs.removeObserver(this, "places-init-complete");
        this._initDefaultBookmarks();
        break;
      case "final-ui-startup":
        Services.obs.removeObserver(this, "final-ui-startup");
        this._startupActions();
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserStartup]);
