



































const EXPORTED_SYMBOLS = ['ClientRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function ClientRecord(uri) {
  CryptoWrapper.call(this, uri);
}
ClientRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Client",
};
