



































const EXPORTED_SYMBOLS = ['ClientRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");

function ClientRecord(uri) {
  WBORecord.call(this, uri);
}
ClientRecord.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.Client",

  deserialize: function ClientRecord_deserialize(json) {
    let data = JSON.parse(json, function(key, val) key == "payload" ?
      unescape(val) : val);
    WBORecord.prototype.deserialize.call(this, data);
  },

  toJSON: function toJSON() {
    let obj = WBORecord.prototype.toJSON.call(this);
    obj.payload = escape(obj.payload);
    return obj;
  },

  
  
  get cleartext() JSON.stringify(this),

  
  encrypt: function ClientRecord_encrypt(passphrase) {},
  decrypt: function ClientRecord_decrypt(passphrase) {}
};
