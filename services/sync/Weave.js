



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-sync/util.js");

const SYNC_PREFS_BRANCH = "services.sync.";
















































function WeaveService() {
  this.wrappedJSObject = this;
  this.ready = false;
}
WeaveService.prototype = {
  classID: Components.ID("{74b89fb0-f200-4ae8-a3ec-dd164117f6de}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  ensureLoaded: function () {
    
    if (!this.fxAccountsEnabled) {
      Cu.import("resource://services-sync/FxaMigrator.jsm");
    }

    Components.utils.import("resource://services-sync/main.js");

    
    Weave.Service;
  },

  whenLoaded: function() {
    if (this.ready) {
      return Promise.resolve();
    }
    let deferred = Promise.defer();

    Services.obs.addObserver(function onReady() {
      Services.obs.removeObserver(onReady, "weave:service:ready");
      deferred.resolve();
    }, "weave:service:ready", false);
    this.ensureLoaded();
    return deferred.promise;
  },

  




  get fxAccountsEnabled() {
    try {
      
      
      let username = Services.prefs.getCharPref(SYNC_PREFS_BRANCH + "username");
      return !username || username.includes('@');
    } catch (_) {
      return true; 
    }
  },

  








  get enabled() {
    let prefs = Services.prefs.getBranch(SYNC_PREFS_BRANCH);
    return prefs.prefHasUserValue("username") &&
           prefs.prefHasUserValue("clusterURL");
  },

  observe: function (subject, topic, data) {
    switch (topic) {
    case "app-startup":
      let os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
      os.addObserver(this, "final-ui-startup", true);
      break;

    case "final-ui-startup":
      
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.timer.initWithCallback({
        notify: function() {
          let isConfigured = false;
          
          let prefs = Services.prefs.getBranch(SYNC_PREFS_BRANCH);
          if (prefs.prefHasUserValue("username")) {
            
            
            
            
            
            Components.utils.import("resource://services-sync/main.js");
            isConfigured = Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED;
          }
          let getHistogramById = Services.telemetry.getHistogramById;
          getHistogramById("WEAVE_CONFIGURED").add(isConfigured);
          if (isConfigured) {
            getHistogramById("WEAVE_CONFIGURED_MASTER_PASSWORD").add(Utils.mpEnabled());
            this.ensureLoaded();
          }
        }.bind(this)
      }, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      break;
    }
  }
};

function AboutWeaveLog() {}
AboutWeaveLog.prototype = {
  classID: Components.ID("{d28f8a0b-95da-48f4-b712-caf37097be41}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return 0;
  },

  newChannel: function(aURI, aLoadInfo) {
    let dir = FileUtils.getDir("ProfD", ["weave", "logs"], true);
    let uri = Services.io.newFileURI(dir);
    let channel = Services.io.newChannelFromURIWithLoadInfo(uri, aLoadInfo);

    channel.originalURI = aURI;

    
    
    let ssm = Cc["@mozilla.org/scriptsecuritymanager;1"]
                .getService(Ci.nsIScriptSecurityManager);
    let principal = ssm.getNoAppCodebasePrincipal(uri);
    channel.owner = principal;
    return channel;
  }
};

const components = [WeaveService, AboutWeaveLog];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
