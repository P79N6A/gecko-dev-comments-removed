



































const EXPORTED_SYMBOLS = ['HistoryRec'];

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

function HistoryRec(uri) {
  this._HistoryRec_init(uri);
}
HistoryRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.History",

  _HistoryRec_init: function HistItem_init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },

  get histUri() this.cleartext.uri,
  set histUri(value) {
    if (typeof(value) == "string")
      this.cleartext.uri = value;
    else
      this.cleartext.uri = value.spec;
  },

  get title() this.cleartext.title,
  set title(value) {
    this.cleartext.title = value;
  },

  get visits() this.cleartext.visits,
  set visits(value) {
    this.cleartext.visits = value;
  }
};
