



"use strict";

Components.utils.import("resource://gre/modules/devtools/event-emitter.js");











const EXPORTED_SYMBOLS = ["Simulator"];

const Simulator = {
  _simulators: {},

  register: function (name, simulator) {
    
    this._simulators[name] = simulator;
    this.emit("register", name);
  },

  unregister: function (name) {
    delete this._simulators[name];
    this.emit("unregister", name);
  },

  availableNames: function () {
    return Object.keys(this._simulators).sort();
  },

  getByName: function (name) {
    return this._simulators[name];
  },
};

EventEmitter.decorate(Simulator);
