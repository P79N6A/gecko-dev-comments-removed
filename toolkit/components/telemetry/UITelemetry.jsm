



"use strict";

const Cu = Components.utils;

this.EXPORTED_SYMBOLS = [
  "UITelemetry",
];

Cu.import("resource://gre/modules/Services.jsm");






this.UITelemetry = Object.freeze({
  _activeSessions: {},
  _measurements: [],

  



  get wrappedJSObject() {
    return this;
  },

  




  _simpleMeasureFunctions: {},

  








  addEvent: function(aAction, aMethod, aTimestamp, aExtras) {
    let sessions = Object.keys(this._activeSessions);
    let aEvent = {
      type: "event",
      action: aAction,
      method: aMethod,
      sessions: sessions,
      timestamp: aTimestamp,
    };

    if (aExtras) {
      aEvent.extras = aExtras;
    }

    this._recordEvent(aEvent);
  },

  


  startSession: function(aName, aTimestamp) {
    if (this._activeSessions[aName]) {
      
      return;
    }
    this._activeSessions[aName] = aTimestamp;
  },

  


  stopSession: function(aName, aReason, aTimestamp) {
    let sessionStart = this._activeSessions[aName];
    delete this._activeSessions[aName];

    if (!sessionStart) {
      Services.console.logStringMessage("UITelemetry error: no session [" + aName + "] to stop!");
      return;
    }

    let aEvent = {
      type: "session",
      name: aName,
      reason: aReason,
      start: sessionStart,
      end: aTimestamp,
    };

    this._recordEvent(aEvent);
  },

  _recordEvent: function(aEvent) {
    this._measurements.push(aEvent);
  },

  





  getSimpleMeasures: function() {
    let result = {};
    for (let name in this._simpleMeasureFunctions) {
      result[name] = this._simpleMeasureFunctions[name]();
    }
    return result;
  },

  








  addSimpleMeasureFunction: function(aName, aFunction) {
    if (aName in this._simpleMeasureFunctions) {
      throw new Error("A simple measurement function is already registered for " + aName);
    }

    if (!aFunction || typeof aFunction !== 'function') {
      throw new Error("addSimpleMeasureFunction called with non-function argument.");
    }

    this._simpleMeasureFunctions[aName] = aFunction;
  },

  removeSimpleMeasureFunction: function(aName) {
    delete this._simpleMeasureFunctions[aName];
  },

  getUIMeasurements: function getUIMeasurements() {
    return this._measurements.slice();
  }
});
