



































const EXPORTED_SYMBOLS = ['Identity', 'ID'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Utils.lazy(this, 'ID', IDManager);


function IDManager() {
  this._ids = {};
  this._aliases = {};
}
IDManager.prototype = {
  get: function IDMgr_get(name) {
    if (this._aliases[name])
      return this._ids[this._aliases[name]];
    else
      return this._ids[name];
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
  this._realm = realm;
  this._username = username;
  this._password = password;
}
Identity.prototype = {
  get realm() { return this._realm; },
  set realm(value) { this._realm = value; },

  get username() { return this._username; },
  set username(value) { this._username = value; },

  get userHash() { return Utils.sha1(this.username); },

  _privkey: null,
  get privkey() { return this._privkey; },
  set privkey(value) { this._privkey = value; },

  
  _pubkey: null,
  get pubkey() { return this._pubkey; },
  set pubkey(value) { this._pubkey = value; },

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
