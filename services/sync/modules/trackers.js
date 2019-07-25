



































const EXPORTED_SYMBOLS = ['Tracker'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");












function Tracker(name) {
  name = name || "Unnamed";
  this.name = this.file = name.toLowerCase();

  this._log = Log4Moz.repository.getLogger("Tracker." + name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log4Moz.Level[level];

  this._score = 0;
  this._ignored = [];
  this.ignoreAll = false;
  this.changedIDs = {};
  this.loadChangedIDs();
}
Tracker.prototype = {
  









  get score() {
    return this._score;
  },

  set score(value) {
    this._score = value;
    Observers.notify("weave:engine:score:updated", this.name);
  },

  
  resetScore: function T_resetScore() {
    this._score = 0;
  },

  saveChangedIDs: function T_saveChangedIDs() {
    Utils.delay(function() {
      Utils.jsonSave("changes/" + this.file, this, this.changedIDs);
    }, 1000, this, "_lazySave");
  },

  loadChangedIDs: function T_loadChangedIDs() {
    Utils.jsonLoad("changes/" + this.file, this, function(json) {
      this.changedIDs = json;
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

  addChangedID: function addChangedID(id, when) {
    if (!id) {
      this._log.warn("Attempted to add undefined ID to tracker");
      return false;
    }
    if (this.ignoreAll || (id in this._ignored))
      return false;

    
    if (when == null)
      when = Math.floor(Date.now() / 1000);

    
    if ((this.changedIDs[id] || -Infinity) < when) {
      this._log.trace("Adding changed ID: " + [id, when]);
      this.changedIDs[id] = when;
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
    if (this.changedIDs[id] != null) {
      this._log.trace("Removing changed ID " + id);
      delete this.changedIDs[id];
      this.saveChangedIDs();
    }
    return true;
  },

  clearChangedIDs: function T_clearChangedIDs() {
    this._log.trace("Clearing changed ID list");
    this.changedIDs = {};
    this.saveChangedIDs();
  }
};
