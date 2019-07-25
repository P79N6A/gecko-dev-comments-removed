



































const EXPORTED_SYMBOLS = ['HistoryRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

const HISTORY_TTL = 5184000; 

function HistoryRec(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
HistoryRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.History",
  ttl: HISTORY_TTL
};

Utils.deferGetSet(HistoryRec, "cleartext", ["histUri", "title", "visits"]);
