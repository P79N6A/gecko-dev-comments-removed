




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

    let manual = document.getElementById("connect");
    manual.addEventListener("click", (evt) => {
      this.connectManually(evt);
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

  _fixedDeviceForType: function(type, ip) {
    let fixedDevice = {};
    if (type == "roku") {
      fixedDevice.target = "roku:ecp";
      fixedDevice.location = "http://" + ip + ":8060";
    } else if (type == "chromecast") {
      fixedDevice.target = "urn:dial-multiscreen-org:service:dial:1";
      fixedDevice.location = "http://" + ip + ":8008";
    }
    return fixedDevice;
  },

  connectManually: function(evt) {
    
    
    let ip = document.getElementById("ip");
    if (!ip.checkValidity()) {
      dump("Manually entered IP address is not valid!");
      return;
    }

    let fixedDevices = [];
    try {
      fixedDevices = JSON.parse(Services.prefs.getCharPref("browser.casting.fixedDevices"));
    } catch (e) {}

    let type = document.getElementById("type").value;
    let fixedDevice = this._fixedDeviceForType(type, ip.value);

    
    if (fixedDevices.indexOf(fixedDevice) > -1)
      return;

    fixedDevices.push(fixedDevice);
    Services.prefs.setCharPref("browser.casting.fixedDevices", JSON.stringify(fixedDevices));

    
    this.updateDeviceList();
  },
};

window.addEventListener("load", Devices.init.bind(Devices), false);
window.addEventListener("unload", Devices.uninit.bind(Devices), false);
