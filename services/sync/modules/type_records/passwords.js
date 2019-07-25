



































const EXPORTED_SYMBOLS = ['LoginRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function LoginRec(uri) {
  CryptoWrapper.call(this, uri);
}
LoginRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Login",
};

Utils.deferGetSet(LoginRec, "cleartext", ["hostname", "formSubmitURL",
  "httpRealm", "username", "password", "usernameField", "passwordField"]);
