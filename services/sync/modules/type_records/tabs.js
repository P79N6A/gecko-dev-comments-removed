



































const EXPORTED_SYMBOLS = ['TabSetRecord'];

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

  addTab: function TabSetRecord_addTab(title, urlHistory, lastUsed) {
    if (!this.cleartext.tabs)
      this.cleartext.tabs = [];
    if (!title) {
      title = "";
    }
    if (!lastUsed) {
      lastUsed = 0;
    }
    if (!urlHistory || urlHistory.length == 0) {
      return;
    }
    this.cleartext.tabs.push( {title: title,
                               urlHistory: urlHistory,
                               lastUsed: lastUsed});
  },

  getAllTabs: function TabSetRecord_getAllTabs() {
    return this.cleartext.tabs;
  },

  setClientName: function TabSetRecord_setClientName(value) {
    this.cleartext.clientName = value;
  },

  getClientName: function TabSetRecord_getClientName() {
    return this.cleartext.clientName;
  },

  toJson: function TabSetRecord_toJson() {
    return this.cleartext;
  },

  fromJson: function TabSetRecord_fromJson(json) {
    this.cleartext = json;
  }
};
