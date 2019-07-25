



































const EXPORTED_SYMBOLS = ['Identity', 'ID'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");


__defineGetter__("Service", function() {
  delete this.Service;
  Cu.import("resource://services-sync/service.js", this);
  return this.Service;
});

XPCOMUtils.defineLazyGetter(this, "ID", function () {
  return new IDManager();
});


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
  this.realm = realm;
  this.username = username;
  this._password = password;
  if (password)
    this._passwordUTF8 = Utils.encodeUTF8(password);
}
Identity.prototype = {
  get password() {
    
    if (this._password == null)
      for each (let login in this._logins)
        if (login.username.toLowerCase() == this.username) {
          this._password = login.password;
          this._passwordUTF8 = Utils.encodeUTF8(login.password);
        }
    return this._password;
  },

  set password(value) {
    this._password = value;
    this._passwordUTF8 = Utils.encodeUTF8(value);
  },

  get passwordUTF8() {
    if (!this._passwordUTF8)
      this.password; 
    return this._passwordUTF8;
  },

  persist: function persist() {
    
    let exists = false;
    for each (let login in this._logins) {
      if (login.username == this.username && login.password == this._password)
        exists = true;
      else
        Services.logins.removeLogin(login);
    }

    
    let log = Log4Moz.repository.getLogger("Identity");
    if (exists) {
      log.trace("Skipping persist: " + this.realm + " for " + this.username);
      return;
    }

    
    log.trace("Persisting " + this.realm + " for " + this.username);
    let nsLoginInfo = new Components.Constructor(
      "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo, "init");
    let newLogin = new nsLoginInfo(PWDMGR_HOST, null, this.realm,
      this.username, this.password, "", "");
    Services.logins.addLogin(newLogin);
  },

  get _logins() Services.logins.findLogins({}, PWDMGR_HOST, null, this.realm)
};
