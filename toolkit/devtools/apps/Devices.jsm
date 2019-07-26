



"use strict";

Components.utils.import("resource:///modules/devtools/shared/event-emitter.js");

const EXPORTED_SYMBOLS = ["Devices"];

let addonInstalled = false;

const Devices = {
  _devices: {},

  get helperAddonInstalled() {
    return addonInstalled;
  },
  set helperAddonInstalled(v) {
    addonInstalled = v;
    this.emit("addon-status-updated", v);
  },

  register: function (name, device) {
    this._devices[name] = device;
    this.emit("register");
  },

  unregister: function (name) {
    delete this._devices[name];
    this.emit("unregister");
  },

  available: function () {
    return Object.keys(this._devices).sort();
  },

  getByName: function (name) {
    return this._devices[name];
  }
};

EventEmitter.decorate(Devices);
