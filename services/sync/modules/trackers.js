



































const EXPORTED_SYMBOLS = ['Tracker'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");












function Tracker() {
  this._init();
}
Tracker.prototype = {
  _logName: "Tracker",
  file: "none",

  _init: function T__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
    this._log.level = Log4Moz.Level[level];

    this._score = 0;
    this._ignored = [];
    this.ignoreAll = false;
    this.loadChangedIDs();
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
    Utils.jsonSave("changes/" + this.file, this, this.changedIDs);
  },

  loadChangedIDs: function T_loadChangedIDs() {
    Utils.jsonLoad("changes/" + this.file, this, function(json) {
      for (let id in json)
        this.changedIDs[id] = 1;
    });
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
    if (this.ignoreAll || (id in this._ignored))
      return false;
    if (!this.changedIDs[id]) {
      this._log.trace("Adding changed ID " + id);
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
    if (this.ignoreAll || (id in this._ignored))
      return false;
    if (this.changedIDs[id]) {
      this._log.trace("Removing changed ID " + id);
      delete this.changedIDs[id];
      this.saveChangedIDs();
    }
    return true;
  },

  clearChangedIDs: function T_clearChangedIDs() {
    this._log.trace("Clearing changed ID list");
    for (let id in this.changedIDs) {
      delete this.changedIDs[id];
    }
    this.saveChangedIDs();
  }
};
