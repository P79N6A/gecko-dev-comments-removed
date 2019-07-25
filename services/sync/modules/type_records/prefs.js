



































const EXPORTED_SYMBOLS = ['PrefRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function PrefRec(uri) {
  CryptoWrapper.call(this, uri);
}
PrefRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Pref",
};

Utils.deferGetSet(PrefRec, "cleartext", ["type", "value"]);
