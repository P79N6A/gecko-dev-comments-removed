




"use strict";

this.EXPORTED_SYMBOLS = ["SimpleServiceDiscovery"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");



let log = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog.d.bind(null, "SSDP");

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

const SSDP_DISCOVER_TIMEOUT = 10000;

const EVENT_SERVICE_FOUND = "ssdp-service-found";
const EVENT_SERVICE_LOST = "ssdp-service-lost";





var SimpleServiceDiscovery = {
  get EVENT_SERVICE_FOUND() { return EVENT_SERVICE_FOUND; },
  get EVENT_SERVICE_LOST() { return EVENT_SERVICE_LOST; },

  _targets: new Map(),
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
      } else if (name.startsWith("SERVER")) {
        service.server = row.substr(8).trim();
      }
    }.bind(this));

    if (service.location && this._targets.has(service.target)) {
      service.location = this._forceTrailingSlash(service.location);

      
      if (!("server" in service)) {
        service.server = null;
      }

      
      
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
    return (network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_WIFI || network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_ETHERNET);
  },

  _search: function _search() {
    
    this._searchShutdown();

    
    if (!this._usingLAN()) {
      return;
    }

    
    
    this._searchTimestamp = Date.now();

    
    
    this._searchFixedTargets();

    
    let socket = Cc["@mozilla.org/network/udp-socket;1"].createInstance(Ci.nsIUDPSocket);
    try {
      socket.init(SSDP_PORT, false);
      socket.asyncListen(this);
    } catch (e) {
      
      
      log("failed to start socket: " + e);
      return;
    }

    this._searchSocket = socket;
    this._searchTimeout.initWithCallback(this._searchShutdown.bind(this), SSDP_DISCOVER_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

    let data = SSDP_DISCOVER_PACKET;
    for (let [key, target] of this._targets) {
      let msgData = data.replace("%SEARCH_TARGET%", target.target);
      try {
        let msgRaw = converter.convertToByteArray(msgData);
        socket.send(SSDP_ADDRESS, SSDP_PORT, msgRaw, msgRaw.length);
      } catch (e) {
        log("failed to convert to byte array: " + e);
      }
    }

    
    this.getAndroidDevices();
  },

  getAndroidDevices: function() {
    Messaging.sendRequestForResult({ type: "MediaPlayer:Get" }).then((result) => {
      for (let id in result.displays) {
        let display = result.displays[id];

        
        let service = {
          location: display.location,
          target: "media:router",
          friendlyName: display.friendlyName,
          uuid: display.uuid,
          manufacturer: display.manufacturer,
          modelName: display.modelName,
          mirror: display.mirror
        };

        this._addService(service);
      }
    });
  },

  _searchFixedTargets: function _searchFixedTargets() {
    let fixedTargets = null;
    try {
      fixedTargets = Services.prefs.getCharPref("browser.casting.fixedTargets");
    } catch (e) {}

    if (!fixedTargets) {
      return;
    }

    fixedTargets = JSON.parse(fixedTargets);
    for (let fixedTarget of fixedTargets) {
      
      if (!"location" in fixedTarget || !"target" in fixedTarget) {
        continue;
      }

      fixedTarget.location = this._forceTrailingSlash(fixedTarget.location);

      let service = {
        location: fixedTarget.location,
        target: fixedTarget.target
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
          Services.obs.notifyObservers(null, EVENT_SERVICE_LOST, service.uuid);
          this._services.delete(service.uuid);
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

  registerTarget: function registerTarget(aTarget) {
    
    if (!("target" in aTarget) || !("factory" in aTarget)) {
      
      throw "Registration requires a target and a location";
    }

    
    if (!this._targets.has(aTarget.target)) {
      this._targets.set(aTarget.target, aTarget);
    }
  },

  unregisterTarget: function unregisterTarget(aTarget) {
    
    if (!("target" in aTarget) || !("factory" in aTarget)) {
      return;
    }

    
    if (this._targets.has(aTarget.target)) {
      this._targets.delete(aTarget.target);
    }
  },

  findAppForService: function findAppForService(aService) {
    if (!aService || !aService.target) {
      return null;
    }

    
    if (this._targets.has(aService.target)) {
      return this._targets.get(aService.target).factory(aService);
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
      let target = this._targets.get(service.target);
      service.extensions = target.extensions;
      service.types = target.types;
      array.push(service);
    }
    return array;
  },

  
  _filterService: function _filterService(aService) {
    let target = this._targets.get(aService.target);
    if (!target) {
      return false;
    }

    
    if (!("filters" in target)) {
      return true;
    }

    
    let filters = target.filters;
    for (let filter in filters) {
      if (filter in aService && aService[filter] != filters[filter]) {
        return false;
      }
    }

    return true;
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

        this._addService(aService);
      }
    }).bind(this), false);

    xhr.send(null);
  },

  _addService: function(service) {
    
    if (!this._filterService(service)) {
      return;
    }

    
    if (!this._services.has(service.uuid)) {
      this._services.set(service.uuid, service);
      Services.obs.notifyObservers(null, EVENT_SERVICE_FOUND, service.uuid);
    }

    
    this._services.get(service.uuid).lastPing = this._searchTimestamp;
  }
}
