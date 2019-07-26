



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
    
    
    let fxAccountsEnabled;
    try {
      fxAccountsEnabled = Services.prefs.getBoolPref("services.sync.fxaccounts.enabled");
    } catch (_) {
      
      
      
      let prefs = Services.prefs.getBranch(SYNC_PREFS_BRANCH);
      fxAccountsEnabled = !prefs.prefHasUserValue("username");
      Services.prefs.setBoolPref("services.sync.fxaccounts.enabled", fxAccountsEnabled);
    }
    
    
    
    return fxAccountsEnabled;
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
          
          let prefs = Services.prefs.getBranch(SYNC_PREFS_BRANCH);
          if (!prefs.prefHasUserValue("username")) {
            return;
          }

          
          
          
          
          
          Components.utils.import("resource://services-sync/main.js");
          if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
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

  newChannel: function(aURI) {
    let dir = FileUtils.getDir("ProfD", ["weave", "logs"], true);
    let uri = Services.io.newFileURI(dir);
    let channel = Services.io.newChannelFromURI(uri);
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
