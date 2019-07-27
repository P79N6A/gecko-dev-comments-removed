



"use strict;"

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "fxAccounts",
  "resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyGetter(this, "WeaveService", function() {
  return Cc["@mozilla.org/weave/service;1"]
         .getService(Components.interfaces.nsISupports)
         .wrappedJSObject;
});

XPCOMUtils.defineLazyModuleGetter(this, "Weave",
  "resource://services-sync/main.js");


let fxAccountsCommon = {};
Cu.import("resource://gre/modules/FxAccountsCommon.js", fxAccountsCommon);


const OBSERVER_STATE_CHANGE_TOPIC = "fxa-migration:state-changed";



const OBSERVER_STATE_REQUEST_TOPIC = "fxa-migration:state-request";



const OBSERVER_INTERNAL_STATE_CHANGE_TOPIC = "fxa-migration:internal-state-changed";



const OBSERVER_INTERNAL_TELEMETRY_TOPIC = "fxa-migration:internal-telemetry";

const OBSERVER_TOPICS = [
  "xpcom-shutdown",
  "weave:service:sync:start",
  "weave:service:sync:finish",
  "weave:service:sync:error",
  "weave:eol",
  OBSERVER_STATE_REQUEST_TOPIC,
  fxAccountsCommon.ONLOGIN_NOTIFICATION,
  fxAccountsCommon.ONLOGOUT_NOTIFICATION,
  fxAccountsCommon.ONVERIFIED_NOTIFICATION,
];



const FXA_SENTINEL_PREFS = [
  "identity.fxaccounts.auth.uri",
  "identity.fxaccounts.remote.force_auth.uri",
  "identity.fxaccounts.remote.signup.uri",
  "identity.fxaccounts.remote.signin.uri",
  "identity.fxaccounts.settings.uri",
  "services.sync.tokenServerURI",
];

function Migrator() {
  
  
  
  this.log.level = Log.Level.Debug;

  this._nextUserStatePromise = Promise.resolve();

  for (let topic of OBSERVER_TOPICS) {
    Services.obs.addObserver(this, topic, false);
  }
  
  
  this._state = null;
}

Migrator.prototype = {
  log: Log.repository.getLogger("Sync.SyncMigration"),

  
  
  
  
  
  
  
  STATE_USER_FXA: "waiting for user to be signed in to FxA",
  STATE_USER_FXA_VERIFIED: "waiting for a verified FxA user",

  
  
  STATE_INTERNAL_WAITING_SYNC_COMPLETE: "waiting for sync to complete",
  STATE_INTERNAL_WAITING_WRITE_SENTINEL: "waiting for sentinel to be written",
  STATE_INTERNAL_WAITING_START_OVER: "waiting for sync to reset itself",
  STATE_INTERNAL_COMPLETE: "migration complete",

  
  
  TELEMETRY_ACCEPTED: "accepted",
  TELEMETRY_DECLINED: "declined",
  TELEMETRY_UNLINKED: "unlinked",

  finalize() {
    for (let topic of OBSERVER_TOPICS) {
      Services.obs.removeObserver(this, topic);
    }
  },

  observe(subject, topic, data) {
    this.log.debug("observed " + topic);
    switch (topic) {
      case "xpcom-shutdown":
        this.finalize();
        break;

      case OBSERVER_STATE_REQUEST_TOPIC:
        
        this._queueCurrentUserState(true);
        break;

      default:
        
        this._queueCurrentUserState().then(
          () => this.log.debug("update state from observer " + topic + " complete")
        ).catch(err => {
          let msg = "Failed to handle topic " + topic + ": " + err;
          Cu.reportError(msg);
          this.log.error(msg);
        });
    }
  },

  
  
  
  
  
  
  
  
  _queueCurrentUserState(forceObserver = false) {
    return this._nextUserStatePromise = this._nextUserStatePromise.then(
      () => this._promiseCurrentUserState(forceObserver),
      err => {
        let msg = "Failed to determine the current user state: " + err;
        Cu.reportError(msg);
        this.log.error(msg);
        return this._promiseCurrentUserState(forceObserver)
      }
    );
  },

  _promiseCurrentUserState: Task.async(function* (forceObserver) {
    this.log.trace("starting _promiseCurrentUserState");
    let update = (newState, email=null) => {
      this.log.info("Migration state: '${state}' => '${newState}'",
                    {state: this._state, newState: newState});
      if (forceObserver || newState !== this._state) {
        this._state = newState;
        let subject = Cc["@mozilla.org/supports-string;1"]
                      .createInstance(Ci.nsISupportsString);
        subject.data = email || "";
        Services.obs.notifyObservers(subject, OBSERVER_STATE_CHANGE_TOPIC, newState);
      }
      return newState;
    }

    
    
    if (WeaveService.fxAccountsEnabled) {
      
      
      this.log.debug("FxA enabled - there's nothing to do!")
      this._unblockSync();
      return update(null);
    }

    
    
    
    let isEOL = false;
    try {
      isEOL = !!Services.prefs.getCharPref("services.sync.errorhandler.alert.mode");
    } catch (e) {}

    if (!isEOL) {
      return update(null);
    }

    
    let fxauser = yield fxAccounts.getSignedInUser();
    if (!fxauser) {
      
      
      
      let sentinel = yield this._getSyncMigrationSentinel();
      return update(this.STATE_USER_FXA, sentinel && sentinel.email);
    }
    if (!fxauser.verified) {
      return update(this.STATE_USER_FXA_VERIFIED, fxauser.email);
    }

    
    
    this.log.info("No next user state - doing some housekeeping");
    update(null);

    
    
    this._blockSync();

    
    if (Weave.Service._locked) {
      
      this.log.info("waiting for sync to complete")
      Services.obs.notifyObservers(null, OBSERVER_INTERNAL_STATE_CHANGE_TOPIC,
                                   this.STATE_INTERNAL_WAITING_SYNC_COMPLETE);
      return null;
    }

    
    Services.obs.notifyObservers(null, OBSERVER_INTERNAL_STATE_CHANGE_TOPIC,
                                 this.STATE_INTERNAL_WAITING_WRITE_SENTINEL);
    yield this._setMigrationSentinelIfNecessary();

    
    let enginePrefs = this._getEngineEnabledPrefs();

    
    this.log.info("Performing final sync migration steps");
    
    
    
    let observeStartOverIdentity;
    Services.obs.addObserver(observeStartOverIdentity = () => {
      this.log.info("observed that startOver is about to re-initialize the identity");
      Services.obs.removeObserver(observeStartOverIdentity, "weave:service:start-over:init-identity");
      
      
      for (let [prefName, prefType, prefVal] of enginePrefs) {
        this.log.debug("Restoring pref ${prefName} (type=${prefType}) to ${prefVal}",
                       {prefName, prefType, prefVal});
        switch (prefType) {
          case Services.prefs.PREF_BOOL:
            Services.prefs.setBoolPref(prefName, prefVal);
            break;
          case Services.prefs.PREF_STRING:
            Services.prefs.setCharPref(prefName, prefVal);
            break;
          default:
            
            Cu.reportError("unknown engine pref type for " + prefName + ": " + prefType);
        }
      }
    }, "weave:service:start-over:init-identity", false);

    
    
    let startOverComplete = new Promise((resolve, reject) => {
      let observe;
      Services.obs.addObserver(observe = () => {
        this.log.info("observed that startOver is complete");
        Services.obs.removeObserver(observe, "weave:service:start-over:finish");
        resolve();
      }, "weave:service:start-over:finish", false);
    });

    Weave.Service.startOver();
    
    Services.obs.notifyObservers(null, OBSERVER_INTERNAL_STATE_CHANGE_TOPIC,
                                 this.STATE_INTERNAL_WAITING_START_OVER);
    yield startOverComplete;
    
    this.log.info("scheduling initial FxA sync.");
    
    
    this._unblockSync();
    Weave.Service.scheduler.scheduleNextSync(0);

    
    
    forceObserver = true;
    this.log.info("Migration complete");
    update(null);

    Services.obs.notifyObservers(null, OBSERVER_INTERNAL_STATE_CHANGE_TOPIC,
                                 this.STATE_INTERNAL_COMPLETE);
    return null;
  }),

  
  _getSentinelPrefs() {
    let result = {};
    for (let pref of FXA_SENTINEL_PREFS) {
      if (Services.prefs.prefHasUserValue(pref)) {
        result[pref] = Services.prefs.getCharPref(pref);
      }
    }
    return result;
  },

  
  _applySentinelPrefs(savedPrefs) {
    for (let pref of FXA_SENTINEL_PREFS) {
      if (savedPrefs[pref]) {
        Services.prefs.setCharPref(pref, savedPrefs[pref]);
      }
    }
  },

  
  _setSyncMigrationSentinel: Task.async(function* () {
    yield WeaveService.whenLoaded();
    let signedInUser = yield fxAccounts.getSignedInUser();
    let sentinel = {
      email: signedInUser.email,
      uid: signedInUser.uid,
      verified: signedInUser.verified,
      prefs: this._getSentinelPrefs(),
    };
    yield Weave.Service.setFxAMigrationSentinel(sentinel);
  }),

  


  _setMigrationSentinelIfNecessary: Task.async(function* () {
    if (!(yield this._getSyncMigrationSentinel())) {
      this.log.info("writing the migration sentinel");
      yield this._setSyncMigrationSentinel();
    }
  }),

  
  _getSyncMigrationSentinel: Task.async(function* () {
    yield WeaveService.whenLoaded();
    let sentinel = yield Weave.Service.getFxAMigrationSentinel();
    this.log.debug("got migration sentinel ${}", sentinel);
    return sentinel;
  }),

  _getDefaultAccountName: Task.async(function* (sentinel) {
    
    
    
    
    
    
    if (sentinel && sentinel.email) {
      this.log.info("defaultAccountName found via sentinel: ${}", sentinel.email);
      return sentinel.email;
    }
    
    let account = Weave.Service.identity.account;
    if (account && account.contains("@")) {
      this.log.info("defaultAccountName found via legacy account name: {}", account);
      return account;
    }
    this.log.info("defaultAccountName could not find an account");
    return null;
  }),

  
  _blockSync() {
    Weave.Service.scheduler.blockSync();
  },

  _unblockSync() {
    Weave.Service.scheduler.unblockSync();
  },

  


  _getEngineEnabledPrefs() {
    let result = [];
    for (let engine of Weave.Service.engineManager.getAll()) {
      let prefName = "services.sync.engine." + engine.prefName;
      let prefVal;
      try {
        prefVal = Services.prefs.getBoolPref(prefName);
        result.push([prefName, Services.prefs.PREF_BOOL, prefVal]);
      } catch (ex) {} 
    }
    
    try {
      let prefName = "services.sync.declinedEngines";
      let prefVal = Services.prefs.getCharPref(prefName);
      result.push([prefName, Services.prefs.PREF_STRING, prefVal]);
    } catch (ex) {}
    return result;
  },

  
  _allEnginesEnabled() {
    return Weave.Service.engineManager.getAll().every(e => e.enabled);
  },

  



  
  
  
  
  
  createFxAccount: Task.async(function* (win) {
    let {url, options} = yield this.getFxAccountCreationOptions();
    win.switchToTabHavingURI(url, true, options);
    
    
    
  }),

  
  
  
  
  
  
  
  
  
  getFxAccountCreationOptions: Task.async(function* (win) {
    
    if (this._state != this.STATE_USER_FXA) {
      this.log.warn("getFxAccountCreationOptions called in an unexpected state: ${}", this._state);
    }
    
    
    
    let sentinel = yield this._getSyncMigrationSentinel();
    if (sentinel && sentinel.prefs) {
      this._applySentinelPrefs(sentinel.prefs);
    }
    
    
    let action = sentinel ? "signin" : "signup";
    
    let email = yield this._getDefaultAccountName(sentinel);
    let tail = email ? "&email=" + encodeURIComponent(email) : "";
    
    tail += "&migration=sync11";
    
    
    let customize = !this._allEnginesEnabled();
    tail += "&customizeSync=" + customize;

    
    
    this.recordTelemetry(this.TELEMETRY_ACCEPTED);
    return {
      url: "about:accounts?action=" + action + tail,
      options: {ignoreFragment: true, replaceQueryString: true}
    };
  }),

  
  
  
  
  
  resendVerificationMail: Task.async(function * (win) {
    
    if (this._state != this.STATE_USER_FXA_VERIFIED) {
      this.log.warn("resendVerificationMail called in an unexpected state: ${}", this._state);
    }
    let ok = true;
    try {
      yield fxAccounts.resendVerificationEmail();
    } catch (ex) {
      this.log.error("Failed to resend verification mail: ${}", ex);
      ok = false;
    }
    this.recordTelemetry(this.TELEMETRY_ACCEPTED);
    let fxauser = yield fxAccounts.getSignedInUser();
    let sb = Services.strings.createBundle("chrome://browser/locale/accounts.properties");

    let heading = ok ?
                  sb.formatStringFromName("verificationSentHeading", [fxauser.email], 1) :
                  sb.GetStringFromName("verificationNotSentHeading");
    let title = sb.GetStringFromName(ok ? "verificationSentTitle" : "verificationNotSentTitle");
    let description = sb.GetStringFromName(ok ? "verificationSentDescription"
                                              : "verificationNotSentDescription");

    let factory = Cc["@mozilla.org/prompter;1"]
                    .getService(Ci.nsIPromptFactory);
    let prompt = factory.getPrompt(win, Ci.nsIPrompt);
    let bag = prompt.QueryInterface(Ci.nsIWritablePropertyBag2);
    bag.setPropertyAsBool("allowTabModal", true);

    prompt.alert(title, heading + "\n\n" + description);
  }),

  
  
  
  
  forgetFxAccount: Task.async(function * () {
    
    if (this._state != this.STATE_USER_FXA_VERIFIED) {
      this.log.warn("forgetFxAccount called in an unexpected state: ${}", this._state);
    }
    return fxAccounts.signOut();
  }),

  recordTelemetry(flag) {
    
    
    switch (flag) {
      case this.TELEMETRY_ACCEPTED:
      case this.TELEMETRY_UNLINKED:
      case this.TELEMETRY_DECLINED:
        Services.obs.notifyObservers(null, OBSERVER_INTERNAL_TELEMETRY_TOPIC, flag);
        break;
      default:
        throw new Error("Unexpected telemetry flag: " + flag);
    }
  }
}


this.EXPORTED_SYMBOLS = ["fxaMigrator"];
let fxaMigrator = new Migrator();
