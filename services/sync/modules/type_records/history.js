



































const EXPORTED_SYMBOLS = ['HistoryRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function HistoryRec(uri) {
  CryptoWrapper.call(this, uri);
}
HistoryRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.History",
};

Utils.deferGetSet(HistoryRec, "cleartext", ["histUri", "title", "visits"]);
