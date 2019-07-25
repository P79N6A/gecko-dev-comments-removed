


































const EXPORTED_SYMBOLS = ["Status"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://gre/modules/Services.jsm");

let Status = {
  _log: Log4Moz.repository.getLogger("Sync.Status"),
  ready: false,

  get service() {
    return this._service;
  },

  set service(code) {
    this._log.debug("Status.service: " + this._service + " => " + code);
    this._service = code;
  },

  get login() {
    return this._login;
  },

  set login(code) {
    this._log.debug("Status.login: " + this._login + " => " + code);
    this._login = code;

    if (code == LOGIN_FAILED_NO_USERNAME || 
        code == LOGIN_FAILED_NO_PASSWORD || 
        code == LOGIN_FAILED_NO_PASSPHRASE) {
      this.service = CLIENT_NOT_CONFIGURED;      
    } else if (code != LOGIN_SUCCEEDED) {
      this.service = LOGIN_FAILED;
    } else {
      this.service = STATUS_OK;
    }
  },

  get sync() {
    return this._sync;
  },

  set sync(code) {
    this._log.debug("Status.sync: " + this._sync + " => " + code);
    this._sync = code;
    this.service = code == SYNC_SUCCEEDED ? STATUS_OK : SYNC_FAILED;
  },

  get engines() {
    return this._engines;
  },

  set engines([name, code]) {
    this._log.debug("Status for engine " + name + ": " + code);
    this._engines[name] = code;

    if (code != ENGINE_SUCCEEDED) {
      this.service = SYNC_FAILED_PARTIAL;
    }
  },

  
  
  toString: function toString() {
    return "<Status" +
           ": login: "   + Status.login +
           ", service: " + Status.service +
           ", sync: "    + Status.sync + ">";
  },

  checkSetup: function checkSetup() {
    
    let prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefService)
                .getBranch(PREFS_BRANCH);
    let username;
    try {
      username = prefs.getCharPref("username");
    } catch(ex) {}
    
    if (!username) {
      Status.login = LOGIN_FAILED_NO_USERNAME;
      return Status.service;
    }

    Cu.import("resource://services-sync/util.js");
    Cu.import("resource://services-sync/identity.js");
    Cu.import("resource://services-sync/record.js");
    if (!Utils.mpLocked()) {
      let id = ID.get("WeaveID");
      if (!id) {
        id = ID.set("WeaveID", new Identity(PWDMGR_PASSWORD_REALM, username));
      }

      if (!id.password) {
        Status.login = LOGIN_FAILED_NO_PASSWORD;
        return Status.service;
      }

      id = ID.get("WeaveCryptoID");
      if (!id) {
        id = ID.set("WeaveCryptoID",
                    new SyncKeyBundle(PWDMGR_PASSPHRASE_REALM, username));
      }

      if (!id.keyStr) {
        Status.login = LOGIN_FAILED_NO_PASSPHRASE;
        return Status.service;
      }
    }
    return Status.service = STATUS_OK;
  },

  resetBackoff: function resetBackoff() {
    this.enforceBackoff = false;
    this.backoffInterval = 0;
    this.minimumNextSync = 0;
  },

  resetSync: function resetSync() {
    
    let logPref = PREFS_BRANCH + "log.logger.status";
    let logLevel = "Trace";
    try {
      logLevel = Services.prefs.getCharPref(logPref);
    } catch (ex) {
      
    }
    this._log.level = Log4Moz.Level[logLevel];

    this._log.info("Resetting Status.");
    this.service = STATUS_OK;
    this._login = LOGIN_SUCCEEDED;
    this._sync = SYNC_SUCCEEDED;
    this._engines = {};
    this.partial = false;
  }
};


Status.resetBackoff();
Status.resetSync();
