



































const EXPORTED_SYMBOLS = ['ClientRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");

Function.prototype.async = Async.sugar;

function ClientRecord(uri) {
  this._ClientRec_init(uri);
}
ClientRecord.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.Client",

  _ClientRec_init: function ClientRec_init(uri) {
    this._WBORec_init(uri);
  },

  
  
  get cleartext() this.serialize(),

  
  encrypt: function ClientRecord_encrypt(passphrase) {},
  decrypt: function ClientRecord_decrypt(passphrase) {}
};
