



































const EXPORTED_SYMBOLS = ['ClientRec'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");

Function.prototype.async = Async.sugar;

function ClientRec(uri, authenticator) {
  this._ClientRec_init(uri, authenticator);
}
ClientRec.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.Client",

  _ClientRec_init: function ClientRec_init(uri, authenticator) {
    this._WBORec_init(uri, authenticator);
  },

  get name() this.payload.name,
  set name(value) {
    this.payload.name = value;
  },

  get type() this.payload.type,
  set type(value) {
    this.payload.type = value;
  }
};
