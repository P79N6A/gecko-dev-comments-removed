



































const EXPORTED_SYMBOLS = ['LoginRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function LoginRec(uri) {
  this._LoginRec_init(uri);
}
LoginRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Login",

  _LoginRec_init: function LoginItem_init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },

  get hostname() this.cleartext.hostname,
  set hostname(value) {
      this.cleartext.hostname = value;
  },

  get formSubmitURL() this.cleartext.formSubmitURL,
  set formSubmitURL(value) {
    this.cleartext.formSubmitURL = value;
  },

  get httpRealm() this.cleartext.httpRealm,
  set httpRealm(value) {
    this.cleartext.httpRealm = value;
  },
  
  get username() this.cleartext.username,
  set username(value) {
    this.cleartext.username = value;
  },
  
  get password() this.cleartext.password,
  set password(value) {
    this.cleartext.password = value;
  },
  
  get usernameField() this.cleartext.usernameField,
  set usernameField(value) {
    this.cleartext.usernameField = value;
  },
  
  get passwordField() this.cleartext.passwordField,
  set passwordField(value) {
    this.cleartext.passwordField = value;
  }
};
