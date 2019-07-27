




"use strict";

this.EXPORTED_SYMBOLS = ["SimpleServiceDiscovery"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

let log = Cu.reportError;

XPCOMUtils.defineLazyGetter(this, "converter", function () {
  let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Ci.nsIScriptableUnicodeConverter);
  conv.charset = "utf8";
  return conv;
});




const SSDP_PORT = 1900;
const SSDP_ADDRESS = "239.255.255.250";

const SSDP_DISCOVER_PACKET =
  "M-SEARCH * HTTP/1.1\r\n" +
  "HOST: " + SSDP_ADDRESS + ":" + SSDP_PORT + "\r\n" +
  "MAN: \"ssdp:discover\"\r\n" +
  "MX: 2\r\n" +
  "ST: %SEARCH_TARGET%\r\n\r\n";

const SSDP_DISCOVER_ATTEMPTS = 3;
const SSDP_DISCOVER_DELAY = 500;
const SSDP_DISCOVER_TIMEOUT_MULTIPLIER = 2;
const SSDP_TRANSMISSION_INTERVAL = 1000;

const EVENT_SERVICE_FOUND = "ssdp-service-found";
const EVENT_SERVICE_LOST = "ssdp-service-lost";





var SimpleServiceDiscovery = {
  get EVENT_SERVICE_FOUND() { return EVENT_SERVICE_FOUND; },
  get EVENT_SERVICE_LOST() { return EVENT_SERVICE_LOST; },

  _devices: new Map(),
  _services: new Map(),
  _searchSocket: null,
  _searchInterval: 0,
  _searchTimestamp: 0,
  _searchTimeout: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  _searchRepeat: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

  _forceTrailingSlash: function(aURL) {
    
    try {
      aURL = Services.io.newURI(aURL, null, null).spec;
    } catch(e) {}
    return aURL;
  },

  
  onPacketReceived: function(aSocket, aMessage) {
    
    
    let response = aMessage.data.split("\n");
    let service = {};
    response.forEach(function(row) {
      let name = row.toUpperCase();
      if (name.startsWith("LOCATION")) {
        service.location = row.substr(10).trim();
      } else if (name.startsWith("ST")) {
        service.target = row.substr(4).trim();
      }
    }.bind(this));

    if (service.location && service.target) {
      service.location = this._forceTrailingSlash(service.location);

      
      
      try {
        this._processService(service);
      } catch (e) {}
    }
  },

  onStopListening: function(aSocket, aStatus) {
    
    
    this._searchTimeout.cancel();
    this._searchSocket = null;
  },

  
  
  
  search: function search(aInterval) {
    let existingSearchInterval = this._searchInterval;
    if (aInterval > 0) {
      this._searchInterval = aInterval || 0;
      this._searchRepeat.initWithCallback(this._search.bind(this), this._searchInterval, Ci.nsITimer.TYPE_REPEATING_SLACK);
    }
    this._search();
    return existingSearchInterval;
  },

  
  stopSearch: function stopSearch() {
    this._searchRepeat.cancel();
  },

  _usingLAN: function() {
    let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
    return (network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_WIFI ||
            network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_ETHERNET ||
            network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_UNKNOWN);
  },

  _search: function _search() {
    
    this._searchShutdown();

    
    if (!this._usingLAN()) {
      return;
    }

    
    
    this._searchTimestamp = Date.now();

    
    
    this._searchFixedDevices();

    
    let socket = Cc["@mozilla.org/network/udp-socket;1"].createInstance(Ci.nsIUDPSocket);
    try {
      socket.init(SSDP_PORT, false, Services.scriptSecurityManager.getSystemPrincipal());
      socket.joinMulticast(SSDP_ADDRESS);
      socket.asyncListen(this);
    } catch (e) {
      
      
      log("failed to start socket: " + e);
      return;
    }

    
    const SSDP_DISCOVER_TIMEOUT = this._devices.size * SSDP_DISCOVER_ATTEMPTS * SSDP_TRANSMISSION_INTERVAL * SSDP_DISCOVER_TIMEOUT_MULTIPLIER;
    this._searchSocket = socket;
    this._searchTimeout.initWithCallback(this._searchShutdown.bind(this), SSDP_DISCOVER_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

    let data = SSDP_DISCOVER_PACKET;

    
    
    let timeout = SSDP_DISCOVER_DELAY;
    for (let attempts = 0; attempts < SSDP_DISCOVER_ATTEMPTS; attempts++) {
      for (let [key, device] of this._devices) {
        let target = device.target;
        setTimeout(function() {
          let msgData = data.replace("%SEARCH_TARGET%", target);
          try {
            let msgRaw = converter.convertToByteArray(msgData);
            socket.send(SSDP_ADDRESS, SSDP_PORT, msgRaw, msgRaw.length);
          } catch (e) {
            log("failed to convert to byte array: " + e);
          }
        }, timeout);
        timeout += SSDP_TRANSMISSION_INTERVAL;
      }
    }
  },

  _searchFixedDevices: function _searchFixedDevices() {
    let fixedDevices = null;
    try {
      fixedDevices = Services.prefs.getCharPref("browser.casting.fixedDevices");
    } catch (e) {}

    if (!fixedDevices) {
      return;
    }

    fixedDevices = JSON.parse(fixedDevices);
    for (let fixedDevice of fixedDevices) {
      
      if (!"location" in fixedDevice || !"target" in fixedDevice) {
        continue;
      }

      fixedDevice.location = this._forceTrailingSlash(fixedDevice.location);

      let service = {
        location: fixedDevice.location,
        target: fixedDevice.target
      };

      
      try {
        this._processService(service);
      } catch (e) {}
    }
  },

  
  
  _searchShutdown: function _searchShutdown() {
    if (this._searchSocket) {
      
      this._searchSocket.close();

      
      for (let [key, service] of this._services) {
        if (service.lastPing != this._searchTimestamp) {
          this.removeService(service.uuid);
        }
      }
    }
  },

  getSupportedExtensions: function() {
    let extensions = [];
    this.services.forEach(function(service) {
        extensions = extensions.concat(service.extensions);
      }, this);
    return extensions.filter(function(extension, pos) {
      return extensions.indexOf(extension) == pos;
    });
  },

  getSupportedMimeTypes: function() {
    let types = [];
    this.services.forEach(function(service) {
        types = types.concat(service.types);
      }, this);
    return types.filter(function(type, pos) {
      return types.indexOf(type) == pos;
    });
  },

  registerDevice: function registerDevice(aDevice) {
    
    if (!("id" in aDevice) || !("target" in aDevice) || !("factory" in aDevice)) {
      
      throw "Registration requires an id, a target and a location";
    }

    
    if (!this._devices.has(aDevice.id)) {
      this._devices.set(aDevice.id, aDevice);
    } else {
      log("device was already registered: " + aDevice.id);
    }
  },

  unregisterDevice: function unregisterDevice(aDevice) {
    
    if (!("id" in aDevice) || !("target" in aDevice) || !("factory" in aDevice)) {
      return;
    }

    
    if (this._devices.has(aDevice.id)) {
      this._devices.delete(aDevice.id);
    } else {
      log("device was not registered: " + aDevice.id);
    }
  },

  findAppForService: function findAppForService(aService) {
    if (!aService || !aService.deviceID) {
      return null;
    }

    
    if (this._devices.has(aService.deviceID)) {
      return this._devices.get(aService.deviceID).factory(aService);
    }
    return null;
  },

  findServiceForID: function findServiceForID(aUUID) {
    if (this._services.has(aUUID)) {
      return this._services.get(aUUID);
    }
    return null;
  },

  
  get services() {
    let array = [];
    for (let [key, service] of this._services) {
      let target = this._devices.get(service.deviceID);
      service.extensions = target.extensions;
      service.types = target.types;
      array.push(service);
    }
    return array;
  },

  
  _filterService: function _filterService(aService) {
    
    for (let [key, device] of this._devices) {
      
      if (device.target != aService.target) {
        continue;
      }

      
      if (!("filters" in device)) {
        aService.deviceID = device.id;
        return true;
      }

      
      let failed = false;
      let filters = device.filters;
      for (let filter in filters) {
        if (filter in aService && aService[filter] != filters[filter]) {
          failed = true;
        }
      }

      
      if (!failed) {
        aService.deviceID = device.id;
        return true;
      }
    }

    
    return false;
  },

  _processService: function _processService(aService) {
    
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", aService.location, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.overrideMimeType("text/xml");

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        let doc = xhr.responseXML;
        aService.appsURL = xhr.getResponseHeader("Application-URL");
        if (aService.appsURL && !aService.appsURL.endsWith("/"))
          aService.appsURL += "/";
        aService.friendlyName = doc.querySelector("friendlyName").textContent;
        aService.uuid = doc.querySelector("UDN").textContent;
        aService.manufacturer = doc.querySelector("manufacturer").textContent;
        aService.modelName = doc.querySelector("modelName").textContent;

        this.addService(aService);
      }
    }).bind(this), false);

    xhr.send(null);
  },

  
  
  _addService: function(service) {
    
    if (!this._filterService(service)) {
      return false;
    }

    let device = this._devices.get(service.target);
    if (device && device.mirror) {
      service.mirror = true;
    }
    this._services.set(service.uuid, service);
    return true;
  },

  addService: function(service) {
    
    if (!this._services.has(service.uuid)) {
      if (!this._addService(service)) {
        return;
      }
      Services.obs.notifyObservers(null, EVENT_SERVICE_FOUND, service.uuid);
    }

    
    this._services.get(service.uuid).lastPing = this._searchTimestamp;
  },

  removeService: function(uuid) {
    Services.obs.notifyObservers(null, EVENT_SERVICE_LOST, uuid);
    this._services.delete(uuid);
  },

  updateService: function(service) {
    if (!this._addService(service)) {
      return;
    }

    
    this._services.get(service.uuid).lastPing = this._searchTimestamp;
  }
}
