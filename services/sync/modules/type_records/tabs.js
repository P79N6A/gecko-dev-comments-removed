



































const EXPORTED_SYMBOLS = ['TabSetRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

function TabSetRecord(uri) {
  this._TabSetRecord_init(uri);
}
TabSetRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Tabs",

  _TabSetRecord_init: function TabSetRecord__init(uri) {
    this._CryptoWrap_init(uri);
    this.cleartext = {
    };
  },
};

Utils.deferGetSet(TabSetRecord, "cleartext", ["clientName", "tabs"]);
