

































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
