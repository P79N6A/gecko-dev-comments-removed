



































const EXPORTED_SYMBOLS = ['FormRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

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

  get name() this.cleartext.name,
  set name(p) {
      this.cleartext.name = p;
  },

  get value() this.cleartext.value,
  set value(p) {
    this.cleartext.value = p;
  }
};
