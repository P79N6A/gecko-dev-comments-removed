



































const EXPORTED_SYMBOLS = ['FormRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/util.js");

const FORMS_TTL = 5184000; 

function FormRec(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
FormRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Form",
  ttl: FORMS_TTL
};

Utils.deferGetSet(FormRec, "cleartext", ["name", "value"]);
