



"use strict";

Components.utils.import("resource:///modules/devtools/shared/event-emitter.js");

const EXPORTED_SYMBOLS = ["Simulator"];

const Simulator = {
  _simulators: {},

  register: function (version, simulator) {
    this._simulators[version] = simulator;
    this.emit("register");
  },

  unregister: function (version) {
    delete this._simulators[version];
    this.emit("unregister");
  },

  availableVersions: function () {
    return Object.keys(this._simulators).sort();
  },

  getByVersion: function (version) {
    return this._simulators[version];
  }
};

EventEmitter.decorate(Simulator);
