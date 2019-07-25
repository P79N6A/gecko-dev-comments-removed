



































const EXPORTED_SYMBOLS = ['FormRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function FormRec(uri) {
  CryptoWrapper.call(this, uri);
}
FormRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Form",
};

Utils.deferGetSet(FormRec, "cleartext", ["name", "value"]);
