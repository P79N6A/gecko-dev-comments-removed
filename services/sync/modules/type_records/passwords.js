



































const EXPORTED_SYMBOLS = ['LoginRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

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
};

Utils.deferGetSet(LoginRec, "cleartext", ["hostname", "formSubmitURL",
  "httpRealm", "username", "password", "usernameField", "passwordField"]);
