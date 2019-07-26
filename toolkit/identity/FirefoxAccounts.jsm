



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
    log.debug("watch: " + aRpCaller.id);
    log.debug("Current rp flows: " + this._rpFlows.size);

    
    let runnable = {
      run: () => {
        this.fxAccountsManager.getAssertion(aRpCaller.audience, {silent:true}).then(
          data => {
            if (data) {
              this.doLogin(aRpCaller.id, data);
            } else {
              this.doLogout(aRpCaller.id);
            }
            this.doReady(aRpCaller.id);
          },
          error => {
            log.error("get silent assertion failed: " + JSON.stringify(error));
            this.doError(aRpCaller.id, error);
          }
        );
      }
    };
    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  


  unwatch: function(aRpCallerId, aTargetMM) {
    log.debug("unwatching: " + aRpCallerId);
    this._rpFlows.delete(aRpCallerId);
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
        log.debug("got assertion for " + rp.audience + ": " + data);
        this.doLogin(aRPId, data);
      },
      error => {
        log.error("get assertion failed: " + JSON.stringify(error));
        
        if (error.details && (error.details.error == "DIALOG_CLOSED_BY_USER")) {
          return this.doCancel(aRPId);
        }
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
        this.fxAccountsManager.signOut().then(() => {
          this.doLogout(aRpCallerId);
        });
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
      log.warn("doLogin found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doLogin(aAssertion);
  },

  doLogout: function doLogout(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doLogout found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doLogout();
  },

  doReady: function doReady(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doReady found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doReady();
  },

  doCancel: function doCancel(aRpCallerId) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doCancel found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doCancel();
  },

  doError: function doError(aRpCallerId, aError) {
    let rp = this._rpFlows.get(aRpCallerId);
    if (!rp) {
      log.warn("doError found no rp to go with callerId " + aRpCallerId);
      return;
    }

    rp.doError(aError);
  }
};

this.FirefoxAccounts = new FxAccountsService();

