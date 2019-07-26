



"use strict";

this.EXPORTED_SYMBOLS = ["FirefoxAccounts"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "objectCopy",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "makeMessageObject",
                                  "resource://gre/modules/identity/IdentityUtils.jsm");




const PREF_LOG_LEVEL = "identity.fxaccounts.loglevel";
try {
  this.LOG_LEVEL =
    Services.prefs.getPrefType(PREF_LOG_LEVEL) == Ci.nsIPrefBranch.PREF_STRING
    && Services.prefs.getCharPref(PREF_LOG_LEVEL);
} catch (e) {
  this.LOG_LEVEL = Log.Level.Error;
}

let log = Log.repository.getLogger("Identity.FxAccounts");
log.level = LOG_LEVEL;
log.addAppender(new Log.ConsoleAppender(new Log.BasicFormatter()));

#ifdef MOZ_B2G
XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsManager",
                                  "resource://gre/modules/FxAccountsManager.jsm",
                                  "FxAccountsManager");
#else
log.warn("The FxAccountsManager is only functional in B2G at this time.");
var FxAccountsManager = null;
#endif

function FxAccountsService() {
  Services.obs.addObserver(this, "quit-application-granted", false);

  
  this.RP = this;

  this._rpFlows = new Map();

  
  this.fxAccountsManager = FxAccountsManager;
}

FxAccountsService.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application-granted":
        Services.obs.removeObserver(this, "quit-application-granted");
        break;
    }
  },

  

















  watch: function watch(aRpCaller) {
    this._rpFlows.set(aRpCaller.id, aRpCaller);
    log.debug("Current rp flows: " + this._rpFlows.size);

    
    let runnable = {
      run: () => {
        this.doReady(aRpCaller.id);
      }
    };
    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  unwatch: function(aRpCaller, aTargetMM) {
    
  },

  









  request: function request(aRPId, aOptions) {
    aOptions = aOptions || {};
    let rp = this._rpFlows.get(aRPId);
    if (!rp) {
      log.error("request() called before watch()");
      return;
    }

    let options = makeMessageObject(rp);
    objectCopy(aOptions, options);

    log.debug("get assertion for " + rp.audience);

    this.fxAccountsManager.getAssertion(rp.audience, options).then(
      data => {
        log.debug("got assertion: " + JSON.stringify(data));
        this.doLogin(aRPId, data);
      },
      error => {
        log.error("get assertion failed: " + JSON.stringify(error));
        this.doError(aRPId, error);
      }
    );
  },

  







  logout: function logout(aRpCallerId) {
    
    
    
    
    
    if (!this._rpFlows.has(aRpCallerId)) {
      log.error("logout() called before watch()");
      return;
    }

    
    let runnable = {
      run: () => {
        this.doLogout(aRpCallerId);
      }
    };
    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  childProcessShutdown: function childProcessShutdown(messageManager) {
    for (let [key,] of this._rpFlows) {
      if (this._rpFlows.get(key)._mm === messageManager) {
        this._rpFlows.delete(key);
      }
    }
  },

  doLogin: function doLogin(aRpCallerId, aAssertion) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doLogin found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogin(aAssertion);
  },

  doLogout: function doLogout(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doLogout found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doLogout();
  },

  doReady: function doReady(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doReady found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doReady();
  },

  doCancel: function doCancel(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doCancel found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doCancel();
  },

  doError: function doError(aRpCallerId, aError) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doCancel found no rp to go with callerId " + aRpCallerId + "\n");
      return;
    }

    rp.doError(aError);
  }
};

this.FirefoxAccounts = new FxAccountsService();

