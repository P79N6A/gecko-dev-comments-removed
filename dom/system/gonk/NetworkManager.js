



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/Promise.jsm");

const NETWORKMANAGER_CONTRACTID = "@mozilla.org/network/manager;1";
const NETWORKMANAGER_CID =
  Components.ID("{33901e46-33b8-11e1-9869-f46d04d25bcc}");

const DEFAULT_PREFERRED_NETWORK_TYPE = Ci.nsINetworkInterface.NETWORK_TYPE_WIFI;

XPCOMUtils.defineLazyGetter(this, "ppmm", function() {
  return Cc["@mozilla.org/parentprocessmessagemanager;1"]
         .getService(Ci.nsIMessageBroadcaster);
});

XPCOMUtils.defineLazyServiceGetter(this, "gDNSService",
                                   "@mozilla.org/network/dns-service;1",
                                   "nsIDNSService");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");

const TOPIC_INTERFACE_REGISTERED     = "network-interface-registered";
const TOPIC_INTERFACE_UNREGISTERED   = "network-interface-unregistered";
const TOPIC_ACTIVE_CHANGED           = "network-active-changed";
const TOPIC_PREF_CHANGED             = "nsPref:changed";
const TOPIC_XPCOM_SHUTDOWN           = "xpcom-shutdown";
const TOPIC_CONNECTION_STATE_CHANGED = "network-connection-state-changed";
const PREF_MANAGE_OFFLINE_STATUS     = "network.gonk.manage-offline-status";
const PREF_NETWORK_DEBUG_ENABLED     = "network.debugging.enabled";

const IPV4_ADDRESS_ANY                 = "0.0.0.0";
const IPV6_ADDRESS_ANY                 = "::0";

const IPV4_MAX_PREFIX_LENGTH           = 32;
const IPV6_MAX_PREFIX_LENGTH           = 128;


const CONNECTION_TYPE_CELLULAR  = 0;
const CONNECTION_TYPE_BLUETOOTH = 1;
const CONNECTION_TYPE_ETHERNET  = 2;
const CONNECTION_TYPE_WIFI      = 3;
const CONNECTION_TYPE_OTHER     = 4;
const CONNECTION_TYPE_NONE      = 5;

let debug;
function updateDebug() {
  let debugPref = false; 
  try {
    debugPref = debugPref || Services.prefs.getBoolPref(PREF_NETWORK_DEBUG_ENABLED);
  } catch (e) {}

  if (debugPref) {
    debug = function(s) {
      dump("-*- NetworkManager: " + s + "\n");
    };
  } else {
    debug = function(s) {};
  }
}
updateDebug();

function defineLazyRegExp(obj, name, pattern) {
  obj.__defineGetter__(name, function() {
    delete obj[name];
    return obj[name] = new RegExp(pattern);
  });
}

function NetworkInterfaceLinks()
{
  this.resetLinks();
}
NetworkInterfaceLinks.prototype = {
  linkRoutes: null,
  gateways: null,
  interfaceName: null,
  extraRoutes: null,

  setLinks: function(linkRoutes, gateways, interfaceName) {
    this.linkRoutes = linkRoutes;
    this.gateways = gateways;
    this.interfaceName = interfaceName;
  },

  resetLinks: function() {
    this.linkRoutes = [];
    this.gateways = [];
    this.interfaceName = "";
    this.extraRoutes = [];
  },

  compareGateways: function(gateways) {
    if (this.gateways.length != gateways.length) {
      return false;
    }

    for (let i = 0; i < this.gateways.length; i++) {
      if (this.gateways[i] != gateways[i]) {
        return false;
      }
    }

    return true;
  }
};





function NetworkManager() {
  this.networkInterfaces = {};
  this.networkInterfaceLinks = {};

  try {
    this._manageOfflineStatus =
      Services.prefs.getBoolPref(PREF_MANAGE_OFFLINE_STATUS);
  } catch(ex) {
    
  }
  Services.prefs.addObserver(PREF_MANAGE_OFFLINE_STATUS, this, false);
  Services.prefs.addObserver(PREF_NETWORK_DEBUG_ENABLED, this, false);
  Services.obs.addObserver(this, TOPIC_XPCOM_SHUTDOWN, false);

  this.setAndConfigureActive();

  ppmm.addMessageListener('NetworkInterfaceList:ListInterface', this);

  
  defineLazyRegExp(this, "REGEXP_IPV4", "^\\d{1,3}(?:\\.\\d{1,3}){3}$");
  defineLazyRegExp(this, "REGEXP_IPV6", "^[\\da-fA-F]{4}(?::[\\da-fA-F]{4}){7}$");
}
NetworkManager.prototype = {
  classID:   NETWORKMANAGER_CID,
  classInfo: XPCOMUtils.generateCI({classID: NETWORKMANAGER_CID,
                                    contractID: NETWORKMANAGER_CONTRACTID,
                                    classDescription: "Network Manager",
                                    interfaces: [Ci.nsINetworkManager]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkManager,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIObserver,
                                         Ci.nsISettingsServiceCallback]),

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case TOPIC_PREF_CHANGED:
        if (data === PREF_NETWORK_DEBUG_ENABLED) {
          updateDebug();
        } else if (data === PREF_MANAGE_OFFLINE_STATUS) {
          this._manageOfflineStatus =
            Services.prefs.getBoolPref(PREF_MANAGE_OFFLINE_STATUS);
          debug(PREF_MANAGE_OFFLINE_STATUS + " has changed to " + this._manageOfflineStatus);
        }
        break;
      case TOPIC_XPCOM_SHUTDOWN:
        Services.obs.removeObserver(this, TOPIC_XPCOM_SHUTDOWN);
        Services.prefs.removeObserver(PREF_MANAGE_OFFLINE_STATUS, this);
        Services.prefs.removeObserver(PREF_NETWORK_DEBUG_ENABLED, this);
        break;
    }
  },

  receiveMessage: function(aMsg) {
    switch (aMsg.name) {
      case "NetworkInterfaceList:ListInterface": {
        let excludeMms = aMsg.json.excludeMms;
        let excludeSupl = aMsg.json.excludeSupl;
        let excludeIms = aMsg.json.excludeIms;
        let excludeDun = aMsg.json.excludeDun;
        let interfaces = [];

        for each (let i in this.networkInterfaces) {
          if ((i.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS && excludeMms) ||
              (i.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL && excludeSupl) ||
              (i.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS && excludeIms) ||
              (i.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN && excludeDun)) {
            continue;
          }

          let ips = {};
          let prefixLengths = {};
          i.getAddresses(ips, prefixLengths);

          interfaces.push({
            state: i.state,
            type: i.type,
            name: i.name,
            ips: ips.value,
            prefixLengths: prefixLengths.value,
            gateways: i.getGateways(),
            dnses: i.getDnses(),
            httpProxyHost: i.httpProxyHost,
            httpProxyPort: i.httpProxyPort
          });
        }
        return interfaces;
      }
    }
  },

  getNetworkId: function(network) {
    let id = "device";
    try {
      if (network instanceof Ci.nsIRilNetworkInterface) {
        let rilNetwork = network.QueryInterface(Ci.nsIRilNetworkInterface);
        id = "ril" + rilNetwork.serviceId;
      }
    } catch (e) {}

    return id + "-" + network.type;
  },

  

  registerNetworkInterface: function(network) {
    if (!(network instanceof Ci.nsINetworkInterface)) {
      throw Components.Exception("Argument must be nsINetworkInterface.",
                                 Cr.NS_ERROR_INVALID_ARG);
    }
    let networkId = this.getNetworkId(network);
    if (networkId in this.networkInterfaces) {
      throw Components.Exception("Network with that type already registered!",
                                 Cr.NS_ERROR_INVALID_ARG);
    }
    this.networkInterfaces[networkId] = network;
    this.networkInterfaceLinks[networkId] = new NetworkInterfaceLinks();

    Services.obs.notifyObservers(network, TOPIC_INTERFACE_REGISTERED, null);
    debug("Network '" + networkId + "' registered.");
  },

  _addSubnetRoutes: function(network) {
    let ips = {};
    let prefixLengths = {};
    let length = network.getAddresses(ips, prefixLengths);
    for (let i = 0; i < length; i++) {
      debug('Adding subnet routes: ' + ips.value[i] + '/' + prefixLengths.value[i]);
      gNetworkService.modifyRoute(Ci.nsINetworkService.MODIFY_ROUTE_ADD,
                                  network.name, ips.value[i], prefixLengths.value[i])
        .catch((aError) => {
          debug("_addSubnetRoutes error: " + aError);
        });
    }
  },

  updateNetworkInterface: function(network) {
    if (!(network instanceof Ci.nsINetworkInterface)) {
      throw Components.Exception("Argument must be nsINetworkInterface.",
                                 Cr.NS_ERROR_INVALID_ARG);
    }
    let networkId = this.getNetworkId(network);
    if (!(networkId in this.networkInterfaces)) {
      throw Components.Exception("No network with that type registered.",
                                 Cr.NS_ERROR_INVALID_ARG);
    }
    debug("Network " + network.type + "/" + network.name +
          " changed state to " + network.state);

    
    
    

    switch (network.state) {
      case Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED:
        gNetworkService.createNetwork(network.name, () => {

          
          
          gNetworkService.removeDefaultRoute(network);

          
          
          gNetworkService.setDNS(network, () => {
            
            if (this.isNetworkTypeMobile(network.type)) {
              let currentInterfaceLinks = this.networkInterfaceLinks[networkId];
              let newLinkRoutes = network.getDnses().concat(network.httpProxyHost);
              
              this._handleGateways(networkId, network.getGateways())
                .then(() => this._updateRoutes(currentInterfaceLinks.linkRoutes,
                                               newLinkRoutes,
                                               network.getGateways(), network.name))
                .then(() => currentInterfaceLinks.setLinks(newLinkRoutes,
                                                           network.getGateways(),
                                                           network.name));
            }

            
            
            if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN) {
              this.setSecondaryDefaultRoute(network);
            }

            this._addSubnetRoutes(network);
            this.setAndConfigureActive();

            
            if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI && this.mRil) {
              for (let i = 0; i < this.mRil.numRadioInterfaces; i++) {
                this.mRil.getRadioInterface(i).updateRILNetworkInterface();
              }
            }

            
            CaptivePortalDetectionHelper
              .notify(CaptivePortalDetectionHelper.EVENT_CONNECT, this.active);

            
            
            Services.obs.notifyObservers(network, TOPIC_CONNECTION_STATE_CHANGED,
                                         this.convertConnectionType(network));
          });
        });

        break;
      case Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED:
        
        if (this.isNetworkTypeMobile(network.type)) {
          this._cleanupAllHostRoutes(networkId);
        }
        
        if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN) {
          this.removeSecondaryDefaultRoute(network);
        }
        
        if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
          gNetworkService.resetRoutingTable(network);
        } else if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
          gNetworkService.removeDefaultRoute(network);
        }
        
        if (this.active && network.type == this.active.type) {
          gNetworkService.clearNetworkProxy();
        }

        
        CaptivePortalDetectionHelper
          .notify(CaptivePortalDetectionHelper.EVENT_DISCONNECT, network);
        this.setAndConfigureActive();

        
        if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI && this.mRil) {
          for (let i = 0; i < this.mRil.numRadioInterfaces; i++) {
            this.mRil.getRadioInterface(i).updateRILNetworkInterface();
          }
        }

        gNetworkService.destroyNetwork(network.name, () => {
          
          
          Services.obs.notifyObservers(network, TOPIC_CONNECTION_STATE_CHANGED,
                                       this.convertConnectionType(network));
        });
        break;
    }
  },

  unregisterNetworkInterface: function(network) {
    if (!(network instanceof Ci.nsINetworkInterface)) {
      throw Components.Exception("Argument must be nsINetworkInterface.",
                                 Cr.NS_ERROR_INVALID_ARG);
    }
    let networkId = this.getNetworkId(network);
    if (!(networkId in this.networkInterfaces)) {
      throw Components.Exception("No network with that type registered.",
                                 Cr.NS_ERROR_INVALID_ARG);
    }

    
    
    if (this.isNetworkTypeMobile(network.type)) {
      this._cleanupAllHostRoutes(networkId);
    }

    delete this.networkInterfaces[networkId];

    Services.obs.notifyObservers(network, TOPIC_INTERFACE_UNREGISTERED, null);
    debug("Network '" + networkId + "' unregistered.");
  },

  _manageOfflineStatus: true,

  networkInterfaces: null,

  networkInterfaceLinks: null,

  _preferredNetworkType: DEFAULT_PREFERRED_NETWORK_TYPE,
  get preferredNetworkType() {
    return this._preferredNetworkType;
  },
  set preferredNetworkType(val) {
    if ([Ci.nsINetworkInterface.NETWORK_TYPE_WIFI,
         Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE].indexOf(val) == -1) {
      throw "Invalid network type";
    }
    this._preferredNetworkType = val;
  },

  active: null,
  _overriddenActive: null,

  overrideActive: function(network) {
    if ([Ci.nsINetworkInterface.NETWORK_TYPE_WIFI,
         Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE].indexOf(val) == -1) {
      throw "Invalid network type";
    }

    this._overriddenActive = network;
    this.setAndConfigureActive();
  },

  _updateRoutes: function(oldLinks, newLinks, gateways, interfaceName) {
    
    function getDifference(base, target) {
      return base.filter(function(i) { return target.indexOf(i) < 0; });
    }

    let addedLinks = getDifference(newLinks, oldLinks);
    let removedLinks = getDifference(oldLinks, newLinks);

    if (addedLinks.length === 0 && removedLinks.length === 0) {
      return Promise.resolve();
    }

    return this._setHostRoutes(false, removedLinks, interfaceName, gateways)
      .then(this._setHostRoutes(true, addedLinks, interfaceName, gateways));
  },

  _setHostRoutes: function(doAdd, ipAddresses, networkName, gateways) {
    let getMaxPrefixLength = (aIp) => {
      return aIp.match(this.REGEXP_IPV4) ? IPV4_MAX_PREFIX_LENGTH : IPV6_MAX_PREFIX_LENGTH;
    }

    let promises = [];

    ipAddresses.forEach((aIpAddress) => {
      let gateway = this.selectGateway(gateways, aIpAddress);
      if (gateway) {
        promises.push((doAdd)
          ? gNetworkService.modifyRoute(Ci.nsINetworkService.MODIFY_ROUTE_ADD,
                                        networkName, aIpAddress,
                                        getMaxPrefixLength(aIpAddress), gateway)
          : gNetworkService.modifyRoute(Ci.nsINetworkService.MODIFY_ROUTE_REMOVE,
                                        networkName, aIpAddress,
                                        getMaxPrefixLength(aIpAddress), gateway));
      }
    });

    return Promise.all(promises);
  },

  isValidatedNetwork: function(network) {
    let isValid = false;
    try {
      isValid = (this.getNetworkId(network) in this.networkInterfaces);
    } catch (e) {
      debug("Invalid network interface: " + e);
    }

    return isValid;
  },

  addHostRoute: function(network, host) {
    if (!this.isValidatedNetwork(network)) {
      return Promise.reject("Invalid network interface.");
    }

    return this.resolveHostname(network, host)
      .then((ipAddresses) => {
        let promises = [];
        let networkId = this.getNetworkId(network);

        ipAddresses.forEach((aIpAddress) => {
          let promise =
            this._setHostRoutes(true, [aIpAddress], network.name, network.getGateways())
              .then(() => this.networkInterfaceLinks[networkId].extraRoutes.push(aIpAddress));

          promises.push(promise);
        });

        return Promise.all(promises);
      });
  },

  removeHostRoute: function(network, host) {
    if (!this.isValidatedNetwork(network)) {
      return Promise.reject("Invalid network interface.");
    }

    return this.resolveHostname(network, host)
      .then((ipAddresses) => {
        let promises = [];
        let networkId = this.getNetworkId(network);

        ipAddresses.forEach((aIpAddress) => {
          let found = this.networkInterfaceLinks[networkId].extraRoutes.indexOf(aIpAddress);
          if (found < 0) {
            return; 
          }

          let promise =
            this._setHostRoutes(false, [aIpAddress], network.name, network.getGateways())
              .then(() => {
                this.networkInterfaceLinks[networkId].extraRoutes.splice(found, 1);
              }, () => {
                
                this.networkInterfaceLinks[networkId].extraRoutes.splice(found, 1);
              });
          promises.push(promise);
        });

        return Promise.all(promises);
      });
  },

  isNetworkTypeSecondaryMobile: function(type) {
    return (type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS ||
            type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL ||
            type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS ||
            type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN);
  },

  isNetworkTypeMobile: function(type) {
    return (type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE ||
            this.isNetworkTypeSecondaryMobile(type));
  },

  _handleGateways: function(networkId, gateways) {
    let currentNetworkLinks = this.networkInterfaceLinks[networkId];
    if (currentNetworkLinks.gateways.length == 0 ||
        currentNetworkLinks.compareGateways(gateways)) {
      return Promise.resolve();
    }

    let currentExtraRoutes = currentNetworkLinks.extraRoutes;
    return this._cleanupAllHostRoutes(networkId)
      .then(() => {
        
        if (currentExtraRoutes.length > 0) {
          this._setHostRoutes(true,
                              currentExtraRoutes,
                              currentNetworkLinks.interfaceName,
                              gateways)
          .then(() => {
            currentNetworkLinks.extraRoutes = currentExtraRoutes;
          });
        }
      });
  },

  _cleanupAllHostRoutes: function(networkId) {
    let currentNetworkLinks = this.networkInterfaceLinks[networkId];
    let hostRoutes = currentNetworkLinks.linkRoutes.concat(
      currentNetworkLinks.extraRoutes);

    if (hostRoutes.length === 0) {
      return Promise.resolve();
    }

    return this._setHostRoutes(false,
                               hostRoutes,
                               currentNetworkLinks.interfaceName,
                               currentNetworkLinks.gateways)
      .catch((aError) => {
        debug("Error (" + aError + ") on _cleanupAllHostRoutes, keep proceeding.");
      })
      .then(() => currentNetworkLinks.resetLinks());
  },

  selectGateway: function(gateways, host) {
    for (let i = 0; i < gateways.length; i++) {
      let gateway = gateways[i];
      if (gateway.match(this.REGEXP_IPV4) && host.match(this.REGEXP_IPV4) ||
          gateway.indexOf(":") != -1 && host.indexOf(":") != -1) {
        return gateway;
      }
    }
    return null;
  },

  setSecondaryDefaultRoute: function(network) {
    let gateways = network.getGateways();
    for (let i = 0; i < gateways.length; i++) {
      let isIPv6 = (gateways[i].indexOf(":") != -1) ? true : false;
      
      
      
      let route = {
        ip: gateways[i],
        prefix: isIPv6 ? IPV6_MAX_PREFIX_LENGTH : IPV4_MAX_PREFIX_LENGTH,
        gateway: isIPv6 ? IPV6_ADDRESS_ANY : IPV4_ADDRESS_ANY
      };
      gNetworkService.addSecondaryRoute(network.name, route);
      
      
      route.ip = isIPv6 ? IPV6_ADDRESS_ANY : IPV4_ADDRESS_ANY;
      route.prefix = 0;
      route.gateway = gateways[i];
      gNetworkService.addSecondaryRoute(network.name, route);
    }
  },

  removeSecondaryDefaultRoute: function(network) {
    let gateways = network.getGateways();
    for (let i = 0; i < gateways.length; i++) {
      let isIPv6 = (gateways[i].indexOf(":") != -1) ? true : false;
      
      let route = {
        ip: isIPv6 ? IPV6_ADDRESS_ANY : IPV4_ADDRESS_ANY,
        prefix: 0,
        gateway: gateways[i]
      };
      gNetworkService.removeSecondaryRoute(network.name, route);

      route.ip = gateways[i];
      route.prefix = isIPv6 ? IPV6_MAX_PREFIX_LENGTH : IPV4_MAX_PREFIX_LENGTH;
      route.gateway = isIPv6 ? IPV6_ADDRESS_ANY : IPV4_ADDRESS_ANY;
      gNetworkService.removeSecondaryRoute(network.name, route);
    }
  },

  


  setAndConfigureActive: function() {
    debug("Evaluating whether active network needs to be changed.");
    let oldActive = this.active;

    if (this._overriddenActive) {
      debug("We have an override for the active network: " +
            this._overriddenActive.name);
      
      if (this.active != this._overriddenActive) {
        this.active = this._overriddenActive;
        this._setDefaultRouteAndProxy(this.active, oldActive);
        Services.obs.notifyObservers(this.active, TOPIC_ACTIVE_CHANGED, null);
      }
      return;
    }

    
    if (this.active &&
        this.active.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED &&
        this.active.type == this._preferredNetworkType) {
      debug("Active network is already our preferred type.");
      this._setDefaultRouteAndProxy(this.active, oldActive);
      return;
    }

    
    this.active = null;

    let defaultDataNetwork;
    for each (let network in this.networkInterfaces) {
      if (network.state != Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
        continue;
      }

      if (network.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
        defaultDataNetwork = network;
      }

      this.active = network;
      if (network.type == this.preferredNetworkType) {
        debug("Found our preferred type of network: " + network.name);
        break;
      }
    }

    if (this.active) {
      
      
      
      if (defaultDataNetwork &&
          this.isNetworkTypeSecondaryMobile(this.active.type) &&
          this.active.type != this.preferredNetworkType) {
        this.active = defaultDataNetwork;
      }
      
      if (!this.isNetworkTypeSecondaryMobile(this.active.type)) {
        this._setDefaultRouteAndProxy(this.active, oldActive);
      }
    }

    if (this.active != oldActive) {
      Services.obs.notifyObservers(this.active, TOPIC_ACTIVE_CHANGED, null);
    }

    if (this._manageOfflineStatus) {
      Services.io.offline = !this.active;
    }
  },

  resolveHostname: function(network, hostname) {
    
    if (!hostname) {
      return Promise.reject(new Error("hostname is empty: " + hostname));
    }

    if (hostname.match(this.REGEXP_IPV4) ||
        hostname.match(this.REGEXP_IPV6)) {
      return Promise.resolve([hostname]);
    }

    
    
    
    let hostResolveWrapper = aNetId => {
      return new Promise((aResolve, aReject) => {
        
        let onLookupComplete = (aRequest, aRecord, aStatus) => {
          if (!Components.isSuccessCode(aStatus)) {
            aReject(new Error("Failed to resolve '" + hostname +
                              "', with status: " + aStatus));
            return;
          }

          let retval = [];
          while (aRecord.hasMore()) {
            retval.push(aRecord.getNextAddrAsString());
          }

          if (!retval.length) {
            aReject(new Error("No valid address after DNS lookup!"));
            return;
          }

          debug("hostname is resolved: " + hostname);
          debug("Addresses: " + JSON.stringify(retval));

          aResolve(retval);
        };

        debug('Calling gDNSService.asyncResolveExtended: ' + aNetId + ', ' + hostname);
        gDNSService.asyncResolveExtended(hostname,
                                         0,
                                         aNetId,
                                         onLookupComplete,
                                         Services.tm.mainThread);
      });
    };

    
    
    return gNetworkService.getNetId(network.name)
      .then(aNetId => hostResolveWrapper(aNetId));
  },

  convertConnectionType: function(network) {
    
    
    
    if (network.type != Ci.nsINetworkInterface.NETWORK_TYPE_WIFI &&
        network.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
      return null;
    }

    if (network.state == Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED) {
      return CONNECTION_TYPE_NONE;
    }

    switch (network.type) {
      case Ci.nsINetworkInterface.NETWORK_TYPE_WIFI:
        return CONNECTION_TYPE_WIFI;
      case Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE:
        return CONNECTION_TYPE_CELLULAR;
    }
  },

  _setDefaultRouteAndProxy: function(network, oldInterface) {
    gNetworkService.setDefaultRoute(network, oldInterface, function(success) {
      if (!success) {
        gNetworkService.destroyNetwork(network, function() {});
        return;
      }
      gNetworkService.setNetworkProxy(network);
    });
  },
};

let CaptivePortalDetectionHelper = (function() {

  const EVENT_CONNECT = "Connect";
  const EVENT_DISCONNECT = "Disconnect";
  let _ongoingInterface = null;
  let _available = ("nsICaptivePortalDetector" in Ci);
  let getService = function() {
    return Cc['@mozilla.org/toolkit/captive-detector;1']
             .getService(Ci.nsICaptivePortalDetector);
  };

  let _performDetection = function(interfaceName, callback) {
    let capService = getService();
    let capCallback = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalCallback]),
      prepare: function() {
        capService.finishPreparation(interfaceName);
      },
      complete: function(success) {
        _ongoingInterface = null;
        callback(success);
      }
    };

    
    if (_ongoingInterface != null) {
      capService.abort(_ongoingInterface);
      _ongoingInterface = null;
    }
    try {
      capService.checkCaptivePortal(interfaceName, capCallback);
      _ongoingInterface = interfaceName;
    } catch (e) {
      debug('Fail to detect captive portal due to: ' + e.message);
    }
  };

  let _abort = function(interfaceName) {
    if (_ongoingInterface !== interfaceName) {
      return;
    }

    let capService = getService();
    capService.abort(_ongoingInterface);
    _ongoingInterface = null;
  };

  return {
    EVENT_CONNECT: EVENT_CONNECT,
    EVENT_DISCONNECT: EVENT_DISCONNECT,
    notify: function(eventType, network) {
      switch (eventType) {
        case EVENT_CONNECT:
          
          if (_available && network &&
              network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
            _performDetection(network.name, function() {
              
              
            });
          }

          break;
        case EVENT_DISCONNECT:
          if (_available &&
              network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
            _abort(network.name);
          }
          break;
      }
    }
  };
}());

XPCOMUtils.defineLazyGetter(NetworkManager.prototype, "mRil", function() {
  try {
    return Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch (e) {}

  return null;
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NetworkManager]);