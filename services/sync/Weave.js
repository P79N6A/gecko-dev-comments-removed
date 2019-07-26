



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
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

  get fxAccountsEnabled() {
    let fxAccountsEnabled = false;
    try {
      fxAccountsEnabled = Services.prefs.getBoolPref("identity.fxaccounts.enabled");
    } catch (_) {
    }
    
    
    delete this.fxAccountsEnabled;
    return this.fxAccountsEnabled = fxAccountsEnabled;
  },

  maybeInitWithFxAccountsAndEnsureLoaded: function() {
    Components.utils.import("resource://services-sync/main.js");
    
    Cu.import("resource://gre/modules/FxAccounts.jsm");

    
    
    
    return fxAccounts.getSignedInUser().then(
      (accountData) => {
        if (accountData) {
          Cu.import("resource://services-sync/browserid_identity.js");
          
          
          
          
          
          Weave.Service.identity = Weave.Status._authManager = new BrowserIDManager(),
          
          
          
          Weave.Service.identity.initWithLoggedInUser().then(function () {
            
            Weave.Service.clusterURL = Weave.Service.identity.clusterURL;
            
            
            if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
              
              Svc.Obs.notify("weave:service:setup-complete");
              
              
              Weave.Utils.nextTick(Weave.Service.sync, Weave.Service);
              this.ensureLoaded();
            }
          }.bind(this));
        } else if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
          
          this.ensureLoaded();
        }
      },
      (err) => {dump("err in getting logged in account "+err.message)}
    ).then(null, (err) => {dump("err in processing logged in account "+err.message)});
  },

  observe: function (subject, topic, data) {
    switch (topic) {
    case "app-startup":
      let os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
      os.addObserver(this, "final-ui-startup", true);
      os.addObserver(this, "fxaccounts:onlogin", true);
      os.addObserver(this, "fxaccounts:onlogout", true);
      break;

    case "final-ui-startup":
      
      this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.timer.initWithCallback({
        notify: function() {
          if (this.fxAccountsEnabled) {
            
            this.maybeInitWithFxAccountsAndEnsureLoaded();
          } else {
            
            
            let prefs = Services.prefs.getBranch(SYNC_PREFS_BRANCH);
            if (!prefs.prefHasUserValue("username")) {
              return;
            }

            
            
            
            
            
            Components.utils.import("resource://services-sync/main.js");
            if (Weave.Status.checkSetup() != Weave.CLIENT_NOT_CONFIGURED) {
              this.ensureLoaded();
            }
          }
        }.bind(this)
      }, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      break;

    case 'fxaccounts:onlogin':
        
        
        
        
        
        Components.utils.import("resource://services-sync/main.js"); 
        Weave.Svc.Prefs.set("firstSync", "resetClient");
        this.maybeInitWithFxAccountsAndEnsureLoaded().then(() => {
          
          
          
          
          
        });
      break;
    case 'fxaccounts:onlogout':
      Components.utils.import("resource://services-sync/main.js"); 
      
      
      
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
