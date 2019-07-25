



































const EXPORTED_SYMBOLS = ['TabSetRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

function TabSetRecord(uri) {
  CryptoWrapper.call(this, uri);
}
TabSetRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Tabs",
};

Utils.deferGetSet(TabSetRecord, "cleartext", ["clientName", "tabs"]);
