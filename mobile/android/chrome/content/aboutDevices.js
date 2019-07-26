




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm"); 
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/SimpleServiceDiscovery.jsm"); 

const EVENT_SERVICE_FOUND = SimpleServiceDiscovery.EVENT_SERVICE_FOUND;
const EVENT_SERVICE_LOST = SimpleServiceDiscovery.EVENT_SERVICE_LOST;




const SEARCH_INTERVAL_IN_MILLISECONDS = 5 * 1000;

function dump(s) {
  Services.console.logStringMessage("aboutDevices :: " + s);
}

var Devices = {
  _savedSearchInterval: -1,

  init: function() {
    dump("Initializing.");
    Services.obs.addObserver(this, EVENT_SERVICE_FOUND, false);
    Services.obs.addObserver(this, EVENT_SERVICE_LOST, false);

    let button = document.getElementById("refresh");
    button.addEventListener("click", () => {
      this.updateDeviceList();
    }, false);

    this._savedSearchInterval = SimpleServiceDiscovery.search(SEARCH_INTERVAL_IN_MILLISECONDS);

    this.updateDeviceList();
  },

  uninit: function() {
    dump("Uninitializing.");
    Services.obs.removeObserver(this, EVENT_SERVICE_FOUND);
    Services.obs.removeObserver(this, EVENT_SERVICE_LOST);

    if (this._savedSearchInterval > 0) {
      SimpleServiceDiscovery.search(this._savedSearchInterval);
    }
  },

  _createItemForDevice: function(device) {
    let item = document.createElement("div");

    let friendlyName = document.createElement("div");
    friendlyName.classList.add("name");
    friendlyName.textContent = device.friendlyName;
    item.appendChild(friendlyName);

    let location = document.createElement("div");
    location.classList.add("location");
    location.textContent = device.location;
    item.appendChild(location);

    return item;
  },

  updateDeviceList: function() {
    let services = SimpleServiceDiscovery.services;
    dump("Updating device list with " + services.length + " services.");

    let list = document.getElementById("devices-list");
    while (list.firstChild) {
      list.removeChild(list.firstChild);
    }

    for (let service of services) {
      let item = this._createItemForDevice(service);
      list.appendChild(item);
    }
  },

  observe: function(subject, topic, data) {
    if (topic == EVENT_SERVICE_FOUND || topic == EVENT_SERVICE_LOST) {
      this.updateDeviceList();
    }
  },
};

window.addEventListener("load", Devices.init.bind(Devices), false);
window.addEventListener("unload", Devices.uninit.bind(Devices), false);
