



































const EXPORTED_SYMBOLS = ['FormRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

function FormRec(uri) {
  CryptoWrapper.call(this, uri);
}
FormRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Form",
};

Utils.deferGetSet(FormRec, "cleartext", ["name", "value"]);
