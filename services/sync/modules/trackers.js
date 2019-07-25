



































const EXPORTED_SYMBOLS = ['Tracker'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;












function Tracker() {
  this._init();
}
Tracker.prototype = {
  _logName: "Tracker",
  file: "none",

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return json;
  },

  _init: function T__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._score = 0;
    this._ignored = [];
    this.loadChangedIDs();
    this.enable();
  },

  









  get score() {
    if (this._score >= 100)
      return 100;
    else
      return this._score;
  },

  
  resetScore: function T_resetScore() {
    this._score = 0;
  },

  








  get changedIDs() {
    let items = {};
    this.__defineGetter__("changedIDs", function() items);
    return items;
  },

  saveChangedIDs: function T_saveChangedIDs() {
    this._log.debug("Saving changed IDs to disk");

    let file = Utils.getProfileFile(
      {path: "weave/changes/" + this.file + ".json",
       autoCreate: true});
    let out = this._json.encode(this.changedIDs);
    let [fos] = Utils.open(file, ">");
    fos.writeString(out);
    fos.close();
  },

  loadChangedIDs: function T_loadChangedIDs() {
    let file = Utils.getProfileFile("weave/changes/" + this.file + ".json");
    if (!file.exists())
      return;

    this._log.debug("Loading previously changed IDs from disk");

    try {
      let [is] = Utils.open(file, "<");
      let json = Utils.readStream(is);
      is.close();

      let ids = this._json.decode(json);
      for (let id in ids) {
        this.changedIDs[id] = 1;
      }
    } catch (e) {
      this._log.warn("Could not load changed IDs from previous session");
      this._log.debug("Exception: " + e);
    }
  },

  
  
  

  ignoreID: function T_ignoreID(id) {
    this.unignoreID(id);
    this._ignored.push(id);
  },

  unignoreID: function T_unignoreID(id) {
    let index = this._ignored.indexOf(id);
    if (index != -1)
      this._ignored.splice(index, 1);
  },

  addChangedID: function T_addChangedID(id) {
    if (!id) {
      this._log.warn("Attempted to add undefined ID to tracker");
      return false;
    }
    if (id in this._ignored)
      return false;
    if (!this.changedIDs[id]) {
      this._log.debug("Adding changed ID " + id);
      this.changedIDs[id] = true;
      this.saveChangedIDs();
    }
    return true;
  },

  removeChangedID: function T_removeChangedID(id) {
    if (!id) {
      this._log.warn("Attempted to remove undefined ID to tracker");
      return false;
    }
    if (id in this._ignored)
      return false;
    if (this.changedIDs[id]) {
      this._log.debug("Removing changed ID " + id);
      delete this.changedIDs[id];
      this.saveChangedIDs();
    }
    return true;
  },

  clearChangedIDs: function T_clearChangedIDs() {
    this._log.debug("Clearing changed ID list");
    for (let id in this.changedIDs) {
      delete this.changedIDs[id];
    }
    this.saveChangedIDs();
  }
};
