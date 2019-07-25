



































const EXPORTED_SYMBOLS = ['FormRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/crypto.js");

function FormRec(uri) {
  this._FormRec_init(uri);
}
FormRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Form",

  _FormRec_init: function FormRec_init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },
};

Utils.deferGetSet(FormRec, "cleartext", ["name", "value"]);
