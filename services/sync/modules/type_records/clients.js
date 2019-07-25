



































const EXPORTED_SYMBOLS = ["ClientsRec"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function ClientsRec(uri) {
  CryptoWrapper.call(this, uri);
}
ClientsRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Clients",
};

Utils.deferGetSet(ClientsRec, "cleartext", ["name", "type", "commands"]);
