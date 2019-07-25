



































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
};

Utils.deferGetSet(HistoryRec, "cleartext", ["histUri", "title", "visits"]);
