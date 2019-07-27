



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
  
  
  
  this.level = Log.Level.Debug;

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
    let update = newState => {
      this.log.info("Migration state: '${state}' => '${newState}'",
                    {state: this._state, newState: newState});
      if (forceObserver || newState !== this._state) {
        this._state = newState;
        Services.obs.notifyObservers(null, OBSERVER_STATE_CHANGE_TOPIC, newState);
      }
      return newState;
    }

    
    
    if (WeaveService.fxAccountsEnabled) {
      
      
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
      return update(this.STATE_USER_FXA);
    }
    if (!fxauser.verified) {
      return update(this.STATE_USER_FXA_VERIFIED);
    }

    
    
    this.log.info("No next user state - doing some housekeeping");
    update(null);

    
    
    this._blockSync();

    
    if (Weave.Service._locked) {
      
      this.log.info("waiting for sync to complete")
      return null;
    }

    
    yield this._setMigrationSentinelIfNecessary();

    
    this.log.info("Performing final sync migration steps");
    
    let startOverComplete = new Promise((resolve, reject) => {
      let observe;
      Services.obs.addObserver(observe = () => {
        this.log.info("observed that startOver is complete");
        Services.obs.removeObserver(observe, "weave:service:start-over:finish");
        resolve();
      }, "weave:service:start-over:finish", false);
    });

    Weave.Service.startOver();
    
    yield startOverComplete;
    
    this.log.info("scheduling initial FxA sync.");
    this._unblockSync();
    Weave.Service.scheduler.scheduleNextSync(0);
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

  



  
  
  
  
  
  createFxAccount: Task.async(function* (win) {
    
    if (this._state != this.STATE_USER_FXA) {
      this.log.warn("createFxAccount called in an unexpected state: ${}", this._state);
    }
    
    
    
    let sentinel = yield this._getSyncMigrationSentinel();
    if (sentinel && sentinel.prefs) {
      this._applySentinelPrefs(sentinel.prefs);
    }
    
    
    let action = sentinel ? "signin" : "signup";
    
    let email = yield this._getDefaultAccountName(sentinel);
    let tail = email ? "&email=" + encodeURIComponent(email) : "";
    win.switchToTabHavingURI("about:accounts?" + action + tail, true,
                             {ignoreFragment: true, replaceQueryString: true});
    
    
    
  }),

  
  
  
  
  
  resendVerificationMail: Task.async(function * () {
    
    if (this._state != this.STATE_USER_FXA_VERIFIED) {
      this.log.warn("createFxAccount called in an unexpected state: ${}", this._state);
    }
    return fxAccounts.resendVerificationEmail();
  }),

  
  
  
  
  forgetFxAccount: Task.async(function * () {
    
    if (this._state != this.STATE_USER_FXA_VERIFIED) {
      this.log.warn("createFxAccount called in an unexpected state: ${}", this._state);
    }
    return fxAccounts.signOut();
  }),

}


this.EXPORTED_SYMBOLS = ["fxaMigrator"];
let fxaMigrator = new Migrator();
