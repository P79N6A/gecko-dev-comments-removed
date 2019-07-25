



































const EXPORTED_SYMBOLS = ['WBORecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

function WBORecord(uri, authenticator) {
  this._WBORec_init(uri, authenticator);
}
WBORecord.prototype = {
  __proto__: Resource.prototype,
  _logName: "Record.WBO",

  _WBORec_init: function WBORec_init(uri, authenticator) {
    this._init(uri, authenticator);
    this.pushFilter(new JsonFilter());
  }
};
