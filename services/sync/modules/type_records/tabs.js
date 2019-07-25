



































const EXPORTED_SYMBOLS = ['TabSetRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function TabSetRecord(uri) {
  CryptoWrapper.call(this, uri);
}
TabSetRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Tabs",
};

Utils.deferGetSet(TabSetRecord, "cleartext", ["clientName", "tabs"]);
