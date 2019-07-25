



































const EXPORTED_SYMBOLS = ['Identity', 'ID'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/ext/Sync.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Utils.lazy(this, 'ID', IDManager);


function IDManager() {
  this._ids = {};
}
IDManager.prototype = {
  get: function IDMgr_get(name) {
    if (name in this._ids)
      return this._ids[name];
    return null;
  },
  set: function IDMgr_set(name, id) {
    this._ids[name] = id;
    return id;
  },
  del: function IDMgr_del(name) {
    delete this._ids[name];
  }
};









function Identity(realm, username, password) {
  this._realm = realm;
  this._username = username;
  this._password = password;
}
Identity.prototype = {
  get realm() this._realm,
  set realm(value) {
    let current = Utils.findPassword(this.realm, this.username);
    if (current)
      this.password = null;

    this._realm = value;

    if (current)
      this.password = current;
  },

  get username() this._username,
  set username(value) {
    let current = Utils.findPassword(this.realm, this.username);
    if (current)
      this.password = null;

    this._username = value;

    if (current)
      this.password = current;
  },

  get userHash() { return Utils.sha1(this.username); },

  get password() {
    
    if (!this._password)
      return this._password = Utils.findPassword(this.realm, this.username);
    return this._password;
  },
  set password(value) {
    Utils.setPassword(this.realm, this.username, value);
  },

  setTempPassword: function Id_setTempPassword(value) {
    this._password = value;
  }
};
