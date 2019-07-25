



































const EXPORTED_SYMBOLS = ['ClientRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");

function ClientRecord(uri) {
  this._ClientRec_init(uri);
}
ClientRecord.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.Client",

  _ClientRec_init: function ClientRec_init(uri) {
    this._WBORec_init(uri);
  },

  _escape: function ClientRecord__escape(toAscii) {
    
    if (this.payload != null)
      for (let [key, val] in Iterator(this.payload))
        this.payload[key] = (toAscii ? escape : unescape)(val);
  },

  serialize: function ClientRecord_serialize() {
    
    this._escape(true);
    let ret = WBORecord.prototype.serialize.apply(this, arguments);

    
    this._escape(false);
    return ret;
  },

  deserialize: function ClientRecord_deserialize(json) {
    
    WBORecord.prototype.deserialize.apply(this, arguments);
    this._escape(false);
  },

  
  
  get cleartext() this.serialize(),

  
  encrypt: function ClientRecord_encrypt(passphrase) {},
  decrypt: function ClientRecord_decrypt(passphrase) {}
};
