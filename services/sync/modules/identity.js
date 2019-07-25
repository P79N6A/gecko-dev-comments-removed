



































const EXPORTED_SYMBOLS = ['Identity', 'ID'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Utils.lazy(this, 'ID', IDManager);


function IDManager() {
  this._ids = {};
  this._aliases = {};
}
IDManager.prototype = {
  get: function IDMgr_get(name) {
    if (name in this._aliases)
      return this._ids[this._aliases[name]];
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
  },
  getAlias: function IDMgr_getAlias(alias) {
    return this._aliases[alias];
  },
  setAlias: function IDMgr_setAlias(name, alias) {
    this._aliases[alias] = name;
  },
  delAlias: function IDMgr_delAlias(alias) {
    delete this._aliases[alias];
  }
};









function Identity(realm, username, password) {
  this.realm     = realm;
  this.username  = username;
  this._password = password;
}
Identity.prototype = {
  realm   : null,

  
  privkey        : null,
  pubkey         : null,
  passphraseSalt : null,
  privkeyWrapIV  : null,

  
  bulkKey : null,
  bulkIV  : null,

  username : null,
  get userHash() { return Utils.sha1(this.username); },

  _password: null,
  get password() {
    if (!this._password)
      return Utils.findPassword(this.realm, this.username);
    return this._password;
  },
  set password(value) {
    Utils.setPassword(this.realm, this.username, value);
  },

  setTempPassword: function Id_setTempPassword(value) {
    this._password = value;
  }
};
