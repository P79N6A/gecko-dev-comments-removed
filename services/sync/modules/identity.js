



































const EXPORTED_SYMBOLS = ['Identity'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");









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

  get userHash() {
    

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
      createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let hasher = Cc["@mozilla.org/security/hash;1"]
      .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    let data = converter.convertToByteArray(this.username, {});
    hasher.update(data, data.length);
    let rawHash = hasher.finish(false);

    
    function toHexString(charCode) {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    let hash = [toHexString(rawHash.charCodeAt(i)) for (i in rawHash)].join("");
    
    return hash;
  },

  _privkey: null,
  get privkey() { return this._privkey; },
  set privkey(value) { this._privkey = value; },

  
  _pubkey: null,
  get pubkey() { return this._pubkey; },
  set pubkey(value) { this._pubkey = value; },

  _password: null,
  get password() {
    if (this._password === null)
      return findPassword(this.realm, this.username);
    return this._password;
  },
  set password(value) {
    setPassword(this.realm, this.username, value);
  },

  setTempPassword: function Id_setTempPassword(value) {
    this._password = value;
  }
};


function findPassword(realm, username) {
  
  let password;
  let lm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  let logins = lm.findLogins({}, 'chrome://sync', null, realm);

  for (let i = 0; i < logins.length; i++) {
    if (logins[i].username == username) {
      password = logins[i].password;
      break;
    }
  }
  return password;
}

function setPassword(realm, username, password) {
  
  let lm = Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
  let logins = lm.findLogins({}, 'chrome://sync', null, realm);
  for(let i = 0; i < logins.length; i++) {
    lm.removeLogin(logins[i]);
  }

  if (!password)
    return;

  
  let nsLoginInfo = new Components.Constructor(
    "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo, "init");
  let login = new nsLoginInfo('chrome://sync', null, realm,
                              username, password, "", "");
  lm.addLogin(login);
}
