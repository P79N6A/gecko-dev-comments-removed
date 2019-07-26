




"use strict";

this.EXPORTED_SYMBOLS = ["SimpleServiceDiscovery"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function log(msg) {
  Services.console.logStringMessage("[SSDP] " + msg);
}

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





var SimpleServiceDiscovery = {
  _targets: new Map(),
  _services: new Map(),
  _localAddress: null,
  _searchSocket: null,
  _searchInterval: 0,
  _searchTimestamp: 0,
  _searchTimeout: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  _searchRepeat: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

  
  onPacketReceived: function(aSocket, aMessage) {
    
    
    let response = aMessage.data.split("\n");
    let location;
    let target;
    let valid = false;
    response.some(function(row) {
      let header = row.toUpperCase();
      if (header.startsWith("LOCATION")) {
        location = row.substr(10).trim();
      } else if (header.startsWith("ST")) {
        target = row.substr(4).trim();
        if (this._targets.has(target)) {
          valid = true;
        }
      }

      if (location && valid) {
        
        
        let service = {
          location: location,
          target: target
        };
        this._found(service);
        return true;
      }
      return false;
    }.bind(this));
  },

  onStopListening: function(aSocket, aStatus) {
    
    
    this._searchTimeout.cancel();
    this._searchSocket = null;
  },

  
  
  search: function search(aInterval) {
    if (aInterval > 0) {
      this._searchInterval = aInterval || 0;
      this._searchRepeat.initWithCallback(this._search.bind(this), this._searchInterval, Ci.nsITimer.TYPE_REPEATING_SLACK);
    }
    this._search();
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

    
    let socket = Cc["@mozilla.org/network/udp-socket;1"].createInstance(Ci.nsIUDPSocket);
    try {
      socket.init(SSDP_PORT, false);
      socket.asyncListen(this);
    } catch (e) {
      
      
      log("failed to start socket: " + e);
      return;
    }

    
    
    this._searchTimestamp = Date.now();

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
  },

  
  
  _searchShutdown: function _searchShutdown() {
    if (this._searchSocket) {
      
      this._searchSocket.close();

      
      for (let [key, service] of this._services) {
        if (service.lastPing != this._searchTimestamp) {
          Services.obs.notifyObservers(null, "ssdp-service-lost", service.location);
          this._services.delete(service.location);
        }
      }
    }
  },

  registerTarget: function registerTarget(aTarget, aAppFactory) {
    
    if (!this._targets.has(aTarget)) {
      this._targets.set(aTarget, { target: aTarget, factory: aAppFactory });
    }
  },

  findAppForService: function findAppForService(aService, aApp) {
    if (!aService || !aService.target) {
      return null;
    }

    
    if (this._targets.has(aService.target)) {
      return this._targets.get(aService.target).factory(aService, aApp);
    }
    return null;
  },

  findServiceForLocation: function findServiceForLocation(aLocation) {
    if (this._services.has(aLocation)) {
      return this._services.get(aLocation);
    }
    return null;
  },

  
  get services() {
    let array = [];
    for (let [key, service] of this._services) {
      array.push(service);
    }
    return array;
  },

  _found: function _found(aService) {
    
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", aService.location, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.overrideMimeType("text/xml");

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        if (!this._localAddress) {
          xhr.channel.QueryInterface(Ci.nsIHttpChannelInternal);
          this._localAddress = xhr.channel.localAddress;
        }
        aService.localAddress = this._localAddress;

        let doc = xhr.responseXML;
        aService.appsURL = xhr.getResponseHeader("Application-URL");
        if (aService.appsURL && !aService.appsURL.endsWith("/"))
          aService.appsURL += "/";
        aService.friendlyName = doc.querySelector("friendlyName").textContent;
        aService.uuid = doc.querySelector("UDN").textContent;
        aService.manufacturer = doc.querySelector("manufacturer").textContent;
        aService.modelName = doc.querySelector("modelName").textContent;

        
        if (!this._services.has(aService.location)) {
          this._services.set(aService.location, aService);
          Services.obs.notifyObservers(null, "ssdp-service-found", aService.location);
        }

        
        this._services.get(aService.location).lastPing = this._searchTimestamp;
      }
    }).bind(this), false);

    xhr.send(null);
  }
}
