

































const EXPORTED_SYMBOLS = ["Status"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");

let Status = {
  get login() this._login,
  set login(code) {
    this._login = code;

    if (code == LOGIN_FAILED_NO_USERNAME || 
        code == LOGIN_FAILED_NO_PASSWORD || 
        code == LOGIN_FAILED_NO_PASSPHRASE)
      this.service = CLIENT_NOT_CONFIGURED;      
    else if (code != LOGIN_SUCCEEDED)
      this.service = LOGIN_FAILED;
    else
      this.service = STATUS_OK;
  },

  get sync() this._sync,
  set sync(code) {
    this._sync = code;
    this.service = code == SYNC_SUCCEEDED ? STATUS_OK : SYNC_FAILED;
  },

  get engines() this._engines,
  set engines([name, code]) {
    this._engines[name] = code;

    if (code != ENGINE_SUCCEEDED)
      this.service = SYNC_FAILED_PARTIAL;
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
    if (!Utils.mpLocked()) {
      let id = ID.get("WeaveID");
      if (!id)
        id = ID.set("WeaveID", new Identity(PWDMGR_PASSWORD_REALM, username));

      if (!id.password) {
        Status.login = LOGIN_FAILED_NO_PASSWORD;
        return Status.service;
      }

      id = ID.get("WeaveCryptoID");
      if (!id)
        id = ID.set("WeaveCryptoID",
                    new Identity(PWDMGR_PASSPHRASE_REALM, username));

      if (!id.password) {
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
    this.service = STATUS_OK;
    this._login = LOGIN_SUCCEEDED;
    this._sync = SYNC_SUCCEEDED;
    this._engines = {};
    this.partial = false;
  },
};


Status.resetBackoff();
Status.resetSync();
