



































const EXPORTED_SYMBOLS = ['PrefRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function PrefRec(uri) {
  this._PrefRec_init(uri);
}
PrefRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Pref",

  _PrefRec_init: function PrefRec_init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },
};

Utils.deferGetSet(PrefRec, "cleartext", ["type", "value"]);
