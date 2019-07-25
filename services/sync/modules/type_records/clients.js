



































const EXPORTED_SYMBOLS = ["ClientsRec"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

const CLIENTS_TTL = 1814400; 

function ClientsRec(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
ClientsRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Clients",
  ttl: CLIENTS_TTL
};

Utils.deferGetSet(ClientsRec, "cleartext", ["name", "type", "commands"]);
