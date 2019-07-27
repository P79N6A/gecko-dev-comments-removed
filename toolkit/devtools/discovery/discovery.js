


"use strict";





























const { Cu, CC, Cc, Ci } = require("chrome");
const EventEmitter = require("devtools/toolkit/event-emitter");
const { setTimeout, clearTimeout } = require("sdk/timers");

const UDPSocket = CC("@mozilla.org/network/udp-socket;1",
                     "nsIUDPSocket",
                     "init");

const SCAN_PORT = 50624;
const UPDATE_PORT = 50625;
const ADDRESS = "224.0.0.115";
const REPLY_TIMEOUT = 5000;

const { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

XPCOMUtils.defineLazyGetter(this, "converter", () => {
  let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
             createInstance(Ci.nsIScriptableUnicodeConverter);
  conv.charset = "utf8";
  return conv;
});

XPCOMUtils.defineLazyGetter(this, "sysInfo", () => {
  return Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
});

XPCOMUtils.defineLazyGetter(this, "libcutils", function () {
  Cu.import("resource://gre/modules/systemlibs.js");
  return libcutils;
});

let logging = Services.prefs.getBoolPref("devtools.discovery.log");
function log(msg) {
  if (logging) {
    console.log("DISCOVERY: " + msg);
  }
}






function Transport(port) {
  EventEmitter.decorate(this);
  try {
    this.socket = new UDPSocket(port, false);
    this.socket.joinMulticast(ADDRESS);
    this.socket.asyncListen(this);
  } catch(e) {
    log("Failed to start new socket: " + e);
  }
}

Transport.prototype = {

  






  send: function(object, port) {
    if (logging) {
      log("Send to " + port + ":\n" + JSON.stringify(object, null, 2));
    }
    let message = JSON.stringify(object);
    let rawMessage = converter.convertToByteArray(message);
    try {
      this.socket.send(ADDRESS, port, rawMessage, rawMessage.length);
    } catch(e) {
      log("Failed to send message: " + e);
    }
  },

  destroy: function() {
    this.socket.close();
  },

  

  onPacketReceived: function(socket, message) {
    let messageData = message.data;
    let object = JSON.parse(messageData);
    object.from = message.fromAddr.address;
    let port = message.fromAddr.port;
    if (port == this.socket.port) {
      log("Ignoring looped message");
      return;
    }
    if (logging) {
      log("Recv on " + this.socket.port + ":\n" +
          JSON.stringify(object, null, 2));
    }
    this.emit("message", object);
  },

  onStopListening: function() {}

};







function LocalDevice() {
  this._name = LocalDevice.UNKNOWN;
  if ("@mozilla.org/settingsService;1" in Cc) {
    this._settings =
      Cc["@mozilla.org/settingsService;1"].getService(Ci.nsISettingsService);
    Services.obs.addObserver(this, "mozsettings-changed", false);
  }
  this._get(); 
}

LocalDevice.SETTING = "devtools.discovery.device";
LocalDevice.UNKNOWN = "unknown";

LocalDevice.prototype = {

  _get: function() {
    if (!this._settings) {
      
      
      this._generate();
      return;
    }
    
    this._settings.createLock().get(LocalDevice.SETTING, {
      handle: (_, name) => {
        if (name && name !== LocalDevice.UNKNOWN) {
          this._name = name;
          log("Device: " + this._name);
          return;
        }
        
        this._generate();
      },
      handleError: () => log("Failed to get device name setting")
    });
  },

  



  _generate: function() {
    if (Services.appinfo.widgetToolkit == "gonk") {
      
      
      
      
      let name = libcutils.property_get("ro.product.device");
      
      let randomID = Math.floor(Math.random() * Math.pow(2, 32));
      
      randomID = ("00000000" + randomID.toString(16)).slice(-8);
      this.name = name + "-" + randomID;
    } else {
      this.name = sysInfo.get("host");
    }
  },

  


  observe: function(subject, topic, data) {
    if (topic !== "mozsettings-changed") {
      return;
    }
    if ("wrappedJSObject" in subject) {
      subject = subject.wrappedJSObject;
    }
    if (subject.key !== LocalDevice.SETTING) {
      return;
    }
    this._name = subject.value;
    log("Device: " + this._name);
  },

  get name() {
    return this._name;
  },

  set name(name) {
    if (!this._settings) {
      this._name = name;
      log("Device: " + this._name);
      return;
    }
    
    
    this._settings.createLock().set(LocalDevice.SETTING, name, {
      handle: () => {},
      handleError: () => log("Failed to set device name setting")
    });
  }

};

function Discovery() {
  EventEmitter.decorate(this);

  this.localServices = {};
  this.remoteServices = {};
  this.device = new LocalDevice();
  this.replyTimeout = REPLY_TIMEOUT;

  
  this._factories = { Transport: Transport };

  this._transports = {
    scan: null,
    update: null
  };
  this._expectingReplies = {
    from: new Set()
  };

  this._onRemoteScan = this._onRemoteScan.bind(this);
  this._onRemoteUpdate = this._onRemoteUpdate.bind(this);
  this._purgeMissingDevices = this._purgeMissingDevices.bind(this);

  Services.obs.addObserver(this, "network-active-changed", false);
}

Discovery.prototype = {

  






  addService: function(service, info) {
    log("ADDING LOCAL SERVICE");
    if (Object.keys(this.localServices).length === 0) {
      this._startListeningForScan();
    }
    this.localServices[service] = info;
  },

  




  removeService: function(service) {
    delete this.localServices[service];
    if (Object.keys(this.localServices).length === 0) {
      this._stopListeningForScan();
    }
  },

  


  scan: function() {
    this._startListeningForUpdate();
    this._waitForReplies();
    
    this._sendStatusTo(SCAN_PORT);
  },

  


  getRemoteDevices: function() {
    let devices = new Set();
    for (let service in this.remoteServices) {
      for (let device in this.remoteServices[service]) {
        devices.add(device);
      }
    }
    return [...devices];
  },

  


  getRemoteDevicesWithService: function(service) {
    let devicesWithService = this.remoteServices[service] || {};
    return Object.keys(devicesWithService);
  },

  



  getRemoteService: function(service, device) {
    let devicesWithService = this.remoteServices[service] || {};
    return devicesWithService[device];
  },

  _waitForReplies: function() {
    clearTimeout(this._expectingReplies.timer);
    this._expectingReplies.from = new Set(this.getRemoteDevices());
    this._expectingReplies.timer =
      setTimeout(this._purgeMissingDevices, this.replyTimeout);
  },

  get Transport() {
    return this._factories.Transport;
  },

  _startListeningForScan: function() {
    if (this._transports.scan) {
      return; 
    }
    log("LISTEN FOR SCAN");
    this._transports.scan = new this.Transport(SCAN_PORT);
    this._transports.scan.on("message", this._onRemoteScan);
  },

  _stopListeningForScan: function() {
    if (!this._transports.scan) {
      return; 
    }
    this._transports.scan.off("message", this._onRemoteScan);
    this._transports.scan.destroy();
    this._transports.scan = null;
  },

  _startListeningForUpdate: function() {
    if (this._transports.update) {
      return; 
    }
    log("LISTEN FOR UPDATE");
    this._transports.update = new this.Transport(UPDATE_PORT);
    this._transports.update.on("message", this._onRemoteUpdate);
  },

  _stopListeningForUpdate: function() {
    if (!this._transports.update) {
      return; 
    }
    this._transports.update.off("message", this._onRemoteUpdate);
    this._transports.update.destroy();
    this._transports.update = null;
  },

  observe: function(subject, topic, data) {
    if (topic !== "network-active-changed") {
      return;
    }
    let activeNetwork = subject;
    if (!activeNetwork) {
      log("No active network");
      return;
    }
    activeNetwork = activeNetwork.QueryInterface(Ci.nsINetworkInterface);
    log("Active network changed to: " + activeNetwork.type);
    
    
    if (activeNetwork.type === Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
      this._restartListening();
    }
  },

  _restartListening: function() {
    if (this._transports.scan) {
      this._stopListeningForScan();
      this._startListeningForScan();
    }
    if (this._transports.update) {
      this._stopListeningForUpdate();
      this._startListeningForUpdate();
    }
  },

  



  get _outgoingTransport() {
    if (this._transports.scan) {
      return this._transports.scan;
    }
    if (this._transports.update) {
      return this._transports.update;
    }
    return null;
  },

  _sendStatusTo: function(port) {
    let status = {
      device: this.device.name,
      services: this.localServices
    };
    this._outgoingTransport.send(status, port);
  },

  _onRemoteScan: function() {
    
    log("GOT SCAN REQUEST");
    this._sendStatusTo(UPDATE_PORT);
  },

  _onRemoteUpdate: function(e, update) {
    log("GOT REMOTE UPDATE");

    let remoteDevice = update.device;
    let remoteHost = update.from;

    
    this._expectingReplies.from.delete(remoteDevice);

    
    for (let service in this.remoteServices) {
      let devicesWithService = this.remoteServices[service];
      let hadServiceForDevice = !!devicesWithService[remoteDevice];
      let haveServiceForDevice = service in update.services;
      
      
      if (hadServiceForDevice && !haveServiceForDevice) {
        delete devicesWithService[remoteDevice];
        log("REMOVED " + service + ", DEVICE " + remoteDevice);
        this.emit(service + "-device-removed", remoteDevice);
      }
    }

    
    for (let service in update.services) {
      
      let newDevice = !this.remoteServices[service] ||
                      !this.remoteServices[service][remoteDevice];

      
      
      let devicesWithService = this.remoteServices[service] || {};
      let oldDeviceInfo = devicesWithService[remoteDevice];

      
      let newDeviceInfo = Cu.cloneInto(update.services[service], {});
      newDeviceInfo.host = remoteHost;
      devicesWithService[remoteDevice] = newDeviceInfo;
      this.remoteServices[service] = devicesWithService;

      
      if (newDevice) {
        log("ADDED " + service + ", DEVICE " + remoteDevice);
        this.emit(service + "-device-added", remoteDevice, newDeviceInfo);
      }

      
      
      if (!newDevice &&
          JSON.stringify(oldDeviceInfo) != JSON.stringify(newDeviceInfo)) {
        log("UPDATED " + service + ", DEVICE " + remoteDevice);
        this.emit(service + "-device-updated", remoteDevice, newDeviceInfo);
      }
    }
  },

  _purgeMissingDevices: function() {
    log("PURGING MISSING DEVICES");
    for (let service in this.remoteServices) {
      let devicesWithService = this.remoteServices[service];
      for (let remoteDevice in devicesWithService) {
        
        
        if (this._expectingReplies.from.has(remoteDevice)) {
          delete devicesWithService[remoteDevice];
          log("REMOVED " + service + ", DEVICE " + remoteDevice);
          this.emit(service + "-device-removed", remoteDevice);
        }
      }
    }
  }

};

let discovery = new Discovery();

module.exports = discovery;
