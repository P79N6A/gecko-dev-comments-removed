



"use strict";

Components.utils.import("resource://gre/modules/devtools/event-emitter.js");

const EXPORTED_SYMBOLS = ["Simulator"];

function getVersionNumber(fullVersion) {
  return fullVersion.match(/(\d+\.\d+)/)[0];
}

const Simulator = {
  _simulators: {},

  register: function (label, simulator) {
    
    let versionNumber = getVersionNumber(label);
    this._simulators[versionNumber] = simulator;
    this.emit("register", versionNumber);
  },

  unregister: function (label) {
    let versionNumber = getVersionNumber(label);
    delete this._simulators[versionNumber];
    this.emit("unregister", versionNumber);
  },

  availableVersions: function () {
    return Object.keys(this._simulators).sort();
  },

  getByVersion: function (version) {
    return this._simulators[version];
  }
};

EventEmitter.decorate(Simulator);
