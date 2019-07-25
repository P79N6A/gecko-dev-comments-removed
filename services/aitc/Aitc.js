



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");

Cu.import("resource://services-common/utils.js");

function AitcService() {
  this.aitc = null;
  this.wrappedJSObject = this;
}
AitcService.prototype = {
  classID: Components.ID("{a3d387ca-fd26-44ca-93be-adb5fda5a78d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsINavHistoryObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        
        
        Services.obs.addObserver(this, "sessionstore-windows-restored", true);
        break;
      case "sessionstore-windows-restored":
        Services.obs.removeObserver(this, "sessionstore-windows-restored");

        
        Cu.import("resource://services-common/preferences.js");
        if (Preferences.get("services.sync.engine.apps", false)) {
          return;
        }
        
        if (!Preferences.get("services.aitc.enabled", false)) {
          return;
        }

        
        
        
        

        if (Preferences.get("apps.enabled", false)) {
          this.start();
          return;
        }

        
        this.DASHBOARD_URL = CommonUtils.makeURI(
          Preferences.get("services.aitc.dashboard.url")
        );
        this.MARKETPLACE_URL = CommonUtils.makeURI(
          Preferences.get("services.aitc.marketplace.url")
        );

        if (this.hasUsedApps()) {
          Preferences.set("apps.enabled", true);
          this.start();
          return;
        }

        
        PlacesUtils.history.addObserver(this, true);
        break;
    }
  },

  start: function start() {
    Cu.import("resource://services-aitc/main.js");
    if (this.aitc) {
      return;
    }

    
    Cu.import("resource://services-common/log4moz.js");
    let root = Log4Moz.repository.getLogger("Service.AITC");
    root.level = Log4Moz.Level[Preferences.get("services.aitc.log.level")];
    if (Preferences.get("services.aitc.log.dump")) {
      root.addAppender(new Log4Moz.DumpAppender());
    }
    this.aitc = new Aitc();
  },

  hasUsedApps: function hasUsedApps() {
    
    
    
    let gh = PlacesUtils.ghistory2;
    if (gh.isVisited(this.DASHBOARD_URL)) {
      return true;
    }
    if (gh.isVisited(this.MARKETPLACE_URL)) {
      return true;
    }
    return false;
  },

  
  onBeforeDeleteURI: function() {},
  onBeginUpdateBatch: function() {},
  onClearHistory: function() {},
  onDeleteURI: function() {},
  onDeleteVisits: function() {},
  onEndUpdateBatch: function() {},
  onPageChanged: function() {},
  onPageExpired: function() {},
  onTitleChanged: function() {},

  onVisit: function onVisit(uri) {
    if (!uri.equals(this.MARKETPLACE_URL) && !uri.equals(this.DASHBOARD_URL)) {
      return;
    }

    PlacesUtils.history.removeObserver(this);
    Preferences.set("apps.enabled", true);
    this.start();
    return;
  },
};

const components = [AitcService];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
