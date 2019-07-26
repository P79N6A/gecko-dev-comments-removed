



"use strict";

#ifndef MERGED_COMPARTMENT

this.EXPORTED_SYMBOLS = [
  "SessionRecorder",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

#endif

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");



const MAX_SESSION_AGE_MS = 7 * 24 * 60 * 60 * 1000; 



































this.SessionRecorder = function (branch) {
  if (!branch) {
    throw new Error("branch argument must be defined.");
  }

  if (!branch.endsWith(".")) {
    throw new Error("branch argument must end with '.': " + branch);
  }

  this._log = Log4Moz.repository.getLogger("Services.DataReporting.SessionRecorder");

  this._prefs = new Preferences(branch);
  this._lastActivityWasInactive = false;
  this._activeTicks = 0;
  this._started = false;

  this._os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);

};

SessionRecorder.prototype = Object.freeze({
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  get _currentIndex() {
    return this._prefs.get("currentIndex", 0);
  },

  set _currentIndex(value) {
    this._prefs.set("currentIndex", value);
  },

  get _prunedIndex() {
    return this._prefs.get("prunedIndex", 0);
  },

  set _prunedIndex(value) {
    this._prefs.set("prunedIndex", value);
  },

  get startDate() {
    return CommonUtils.getDatePref(this._prefs, "current.startTime");
  },

  set _startDate(value) {
    CommonUtils.setDatePref(this._prefs, "current.startTime", value);
  },

  get activeTicks() {
    return this._prefs.get("current.activeTicks", 0);
  },

  incrementActiveTicks: function () {
    this._prefs.set("current.activeTicks", ++this._activeTicks);
  },

  get totalTime() {
    return this._prefs.get("current.totalTime", 0);
  },

  updateTotalTime: function () {
    this._prefs.set("current.totalTime", Date.now() - this.startDate);
  },

  get main() {
    return this._prefs.get("current.main", -1);
  },

  set _main(value) {
    if (!Number.isInteger(value)) {
      throw new Error("main time must be an integer.");
    }

    this._prefs.set("current.main", value);
  },

  get firstPaint() {
    return this._prefs.get("current.firstPaint", -1);
  },

  set _firstPaint(value) {
    if (!Number.isInteger(value)) {
      throw new Error("firstPaint must be an integer.");
    }

    this._prefs.set("current.firstPaint", value);
  },

  get sessionRestored() {
    return this._prefs.get("current.sessionRestored", -1);
  },

  set _sessionRestored(value) {
    if (!Number.isInteger(value)) {
      throw new Error("sessionRestored must be an integer.");
    }

    this._prefs.set("current.sessionRestored", value);
  },

  getPreviousSessions: function () {
    let result = {};

    for (let i = this._prunedIndex; i < this._currentIndex; i++) {
      let s = this.getPreviousSession(i);
      if (!s) {
        continue;
      }

      result[i] = s;
    }

    return result;
  },

  getPreviousSession: function (index) {
    return this._deserialize(this._prefs.get("previous." + index));
  },

  



  pruneOldSessions: function (date) {
    for (let i = this._prunedIndex; i < this._currentIndex; i++) {
      let s = this.getPreviousSession(i);
      if (!s) {
        continue;
      }

      if (s.startDate >= date) {
        continue;
      }

      this._log.debug("Pruning session #" + i + ".");
      this._prefs.reset("previous." + i);
      this._prunedIndex = i;
    }
  },

  recordStartupFields: function () {
    let si = this._getStartupInfo();

    if (!si.process) {
      throw new Error("Startup info not available.");
    }

    for (let field of ["main", "firstPaint", "sessionRestored"]) {
      if (!(field in si)) {
        continue;
      }

      this["_" + field] = si[field].getTime() - si.process.getTime();
    }
  },

  




  onStartup: function () {
    if (this._started) {
      throw new Error("onStartup has already been called.");
    }

    let si = this._getStartupInfo();
    if (!si.process) {
      throw new Error("Process information not available. Misconfigured app?");
    }

    this._started = true;

    this._os.addObserver(this, "profile-before-change", false);
    this._os.addObserver(this, "user-interaction-active", false);
    this._os.addObserver(this, "user-interaction-inactive", false);
    this._os.addObserver(this, "idle-daily", false);

    
    this._moveCurrentToPrevious();

    this._startDate = si.process;
    this._prefs.set("current.activeTicks", 0);
    this.updateTotalTime();

    this.recordStartupFields();
  },

  


  onActivity: function (active) {
    let updateActive = active && !this._lastActivityWasInactive;
    this._lastActivityWasInactive = !active;

    this.updateTotalTime();

    if (updateActive) {
      this.incrementActiveTicks();
    }
  },

  onShutdown: function () {
    this._log.info("Recording clean session shutdown.");
    this._prefs.set("current.clean", true);
    this.updateTotalTime();

    this._os.removeObserver(this, "profile-before-change");
    this._os.removeObserver(this, "user-interaction-active");
    this._os.removeObserver(this, "user-interaction-inactive");
    this._os.removeObserver(this, "idle-daily");
  },

  _CURRENT_PREFS: [
    "current.startTime",
    "current.activeTicks",
    "current.totalTime",
    "current.main",
    "current.firstPaint",
    "current.sessionRestored",
    "current.clean",
  ],

  
  _moveCurrentToPrevious: function () {
    try {
      if (!this.startDate.getTime()) {
        this._log.info("No previous session. Is this first app run?");
        return;
      }

      let clean = this._prefs.get("current.clean", false);

      let count = this._currentIndex++;
      let obj = {
        s: this.startDate.getTime(),
        a: this.activeTicks,
        t: this.totalTime,
        c: clean,
        m: this.main,
        fp: this.firstPaint,
        sr: this.sessionRestored,
      };

      this._log.debug("Recording last sessions as #" + count + ".");
      this._prefs.set("previous." + count, JSON.stringify(obj));
    } catch (ex) {
      this._log.warn("Exception when migrating last session: " +
                     CommonUtils.exceptionStr(ex));
    } finally {
      this._log.debug("Resetting prefs from last session.");
      for (let pref of this._CURRENT_PREFS) {
        this._prefs.reset(pref);
      }
    }
  },

  _deserialize: function (s) {
    let o;
    try {
      o = JSON.parse(s);
    } catch (ex) {
      return null;
    }

    return {
      startDate: new Date(o.s),
      activeTicks: o.a,
      totalTime: o.t,
      clean: !!o.c,
      main: o.m,
      firstPaint: o.fp,
      sessionRestored: o.sr,
    };
  },

  
  _getStartupInfo: function () {
    return Cc["@mozilla.org/toolkit/app-startup;1"]
             .getService(Ci.nsIAppStartup)
             .getStartupInfo();
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case "profile-before-change":
        this.onShutdown();
        break;

      case "user-interaction-active":
        this.onActivity(true);
        break;

      case "user-interaction-inactive":
        this.onActivity(false);
        break;

      case "idle-daily":
        this.pruneOldSessions(new Date(Date.now() - MAX_SESSION_AGE_MS));
        break;
    }
  },
});
