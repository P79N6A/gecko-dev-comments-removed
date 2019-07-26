



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const NETWORKSERVICE_CONTRACTID = "@mozilla.org/network/service;1";
const NETWORKSERVICE_CID = Components.ID("{a6c58260-46df-11e3-8f96-0800200c9a66}");


const NETD_COMMAND_PROCEEDING   = 100;

const NETD_COMMAND_OKAY         = 200;


const NETD_COMMAND_FAIL         = 400;

const NETD_COMMAND_ERROR        = 500;

const NETD_COMMAND_UNSOLICITED  = 600;

const WIFI_CTRL_INTERFACE = "wl0.1";

const MANUAL_PROXY_CONFIGURATION = 1;

const DEBUG = false;

function netdResponseType(code) {
  return Math.floor(code / 100) * 100;
}

function isError(code) {
  let type = netdResponseType(code);
  return (type !== NETD_COMMAND_PROCEEDING && type !== NETD_COMMAND_OKAY);
}

function debug(msg) {
  dump("-*- NetworkService: " + msg + "\n");
}





function NetworkService() {
  if(DEBUG) debug("Starting net_worker.");
  this.worker = new ChromeWorker("resource://gre/modules/net_worker.js");
  this.worker.onmessage = this.handleWorkerMessage.bind(this);
  this.worker.onerror = function onerror(event) {
    if(DEBUG) debug("Received error from worker: " + event.filename + 
                    ":" + event.lineno + ": " + event.message + "\n");
    
    event.preventDefault();
  };

  
  this.controlCallbacks = Object.create(null);
}

NetworkService.prototype = {
  classID:   NETWORKSERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: NETWORKSERVICE_CID,
                                    contractID: NETWORKSERVICE_CONTRACTID,
                                    classDescription: "Network Service",
                                    interfaces: [Ci.nsINetworkService]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkService,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIWorkerHolder]),

  

  worker: null,

  

  idgen: 0,
  controlMessage: function controlMessage(params, callback) {
    if (callback) {
      let id = this.idgen++;
      params.id = id;
      this.controlCallbacks[id] = callback;
    }
    this.worker.postMessage(params);
  },

  handleWorkerMessage: function handleWorkerMessage(e) {
    if(DEBUG) debug("NetworkManager received message from worker: " + JSON.stringify(e.data));
    let response = e.data;
    let id = response.id;
    if (id === 'broadcast') {
      Services.obs.notifyObservers(null, response.topic, response.reason);
      return;
    }
    let callback = this.controlCallbacks[id];
    if (callback) {
      callback.call(this, response);
      delete this.controlCallbacks[id];
    }
  },

  

  getNetworkInterfaceStats: function getNetworkInterfaceStats(networkName, callback) {
    if(DEBUG) debug("getNetworkInterfaceStats for " + networkName);

    let params = {
      cmd: "getNetworkInterfaceStats",
      ifname: networkName
    };

    params.report = true;
    params.isAsync = true;

    this.controlMessage(params, function(result) {
      let success = result.resultCode >= NETD_COMMAND_OKAY &&
                    result.resultCode < NETD_COMMAND_ERROR;
      callback.networkStatsAvailable(success, result.rxBytes,
                                     result.txBytes, result.date);
    });
  },

  setWifiOperationMode: function setWifiOperationMode(interfaceName, mode, callback) {
    if(DEBUG) debug("setWifiOperationMode on " + interfaceName + " to " + mode);

    let params = {
      cmd: "setWifiOperationMode",
      ifname: interfaceName,
      mode: mode
    };

    params.report = true;
    params.isAsync = true;

    this.controlMessage(params, function(result) {
      if (isError(result.resultCode)) {
        callback.wifiOperationModeResult("netd command error");
      } else {
        callback.wifiOperationModeResult(null);
      }
    });
  },

  resetRoutingTable: function resetRoutingTable(network) {
    if (!network.ip || !network.netmask) {
      if(DEBUG) debug("Either ip or netmask is null. Cannot reset routing table.");
      return;
    }
    let options = {
      cmd: "removeNetworkRoute",
      ifname: network.name,
      ip: network.ip,
      netmask: network.netmask
    };
    this.worker.postMessage(options);
  },

  setDNS: function setDNS(networkInterface) {
    if(DEBUG) debug("Going DNS to " + networkInterface.name);
    let options = {
      cmd: "setDNS",
      ifname: networkInterface.name,
      dns1_str: networkInterface.dns1,
      dns2_str: networkInterface.dns2
    };
    this.worker.postMessage(options);
  },

  setDefaultRouteAndDNS: function setDefaultRouteAndDNS(network, oldInterface) {
    if(DEBUG) debug("Going to change route and DNS to " + network.name);
    let options = {
      cmd: "setDefaultRouteAndDNS",
      ifname: network.name,
      oldIfname: (oldInterface && oldInterface !== network) ? oldInterface.name : null,
      gateway_str: network.gateway,
      dns1_str: network.dns1,
      dns2_str: network.dns2
    };
    this.worker.postMessage(options);
    this.setNetworkProxy(network);
  },

  removeDefaultRoute: function removeDefaultRoute(ifname) {
    if(DEBUG) debug("Remove default route for " + ifname);
    let options = {
      cmd: "removeDefaultRoute",
      ifname: ifname
    };
    this.worker.postMessage(options);
  },

  addHostRoute: function addHostRoute(network) {
    if(DEBUG) debug("Going to add host route on " + network.name);
    let options = {
      cmd: "addHostRoute",
      ifname: network.name,
      gateway: network.gateway,
      hostnames: [network.dns1, network.dns2, network.httpProxyHost]
    };
    this.worker.postMessage(options);
  },

  removeHostRoute: function removeHostRoute(network) {
    if(DEBUG) debug("Going to remove host route on " + network.name);
    let options = {
      cmd: "removeHostRoute",
      ifname: network.name,
      gateway: network.gateway,
      hostnames: [network.dns1, network.dns2, network.httpProxyHost]
    };
    this.worker.postMessage(options);
  },

  removeHostRoutes: function removeHostRoutes(ifname) {
    if(DEBUG) debug("Going to remove all host routes on " + ifname);
    let options = {
      cmd: "removeHostRoutes",
      ifname: ifname,
    };
    this.worker.postMessage(options);
  },

  addHostRouteWithResolve: function addHostRouteWithResolve(network, hosts) {
    if(DEBUG) debug("Going to add host route after dns resolution on " + network.name);
    let options = {
      cmd: "addHostRoute",
      ifname: network.name,
      gateway: network.gateway,
      hostnames: hosts
    };
    this.worker.postMessage(options);
  },

  removeHostRouteWithResolve: function removeHostRouteWithResolve(network, hosts) {
    if(DEBUG) debug("Going to remove host route after dns resolution on " + network.name);
    let options = {
      cmd: "removeHostRoute",
      ifname: network.name,
      gateway: network.gateway,
      hostnames: hosts
    };
    this.worker.postMessage(options);
  },

  setNetworkProxy: function setNetworkProxy(network) {
    try {
      if (!network.httpProxyHost || network.httpProxyHost === "") {
        
        Services.prefs.clearUserPref("network.proxy.type");
        Services.prefs.clearUserPref("network.proxy.share_proxy_settings");
        Services.prefs.clearUserPref("network.proxy.http");
        Services.prefs.clearUserPref("network.proxy.http_port");
        Services.prefs.clearUserPref("network.proxy.ssl");
        Services.prefs.clearUserPref("network.proxy.ssl_port");
        if(DEBUG) debug("No proxy support for " + network.name + " network interface.");
        return;
      }

      if(DEBUG) debug("Going to set proxy settings for " + network.name + " network interface.");
      
      Services.prefs.setIntPref("network.proxy.type", MANUAL_PROXY_CONFIGURATION);
      
      Services.prefs.setBoolPref("network.proxy.share_proxy_settings", false);
      Services.prefs.setCharPref("network.proxy.http", network.httpProxyHost);
      Services.prefs.setCharPref("network.proxy.ssl", network.httpProxyHost);
      let port = network.httpProxyPort === 0 ? 8080 : network.httpProxyPort;
      Services.prefs.setIntPref("network.proxy.http_port", port);
      Services.prefs.setIntPref("network.proxy.ssl_port", port);
    } catch(ex) {
        if(DEBUG) debug("Exception " + ex + ". Unable to set proxy setting for " +
                         network.name + " network interface.");
    }
  },

  
  setDhcpServer: function setDhcpServer(enabled, config, callback) {
    if (null === config) {
      config = {};
    }

    config.cmd = "setDhcpServer";
    config.isAsync = true;
    config.enabled = enabled;

    this.controlMessage(config, function setDhcpServerResult(response) {
      if (!response.success) {
        callback.dhcpServerResult('Set DHCP server error');
        return;
      }
      callback.dhcpServerResult(null);
    });
  },

  
  setWifiTethering: function setWifiTethering(enable, config, callback) {
    
    
    
    
    config.wifictrlinterfacename = WIFI_CTRL_INTERFACE;
    config.cmd = "setWifiTethering";

    
    config.isAsync = true;
    this.controlMessage(config, function setWifiTetheringResult(data) {
      let code = data.resultCode;
      let reason = data.resultReason;
      let enable = data.enable;
      let enableString = enable ? "Enable" : "Disable";

      if(DEBUG) debug(enableString + " Wifi tethering result: Code " + code + " reason " + reason);

      if (isError(code)) {
        callback.wifiTetheringEnabledChange("netd command error");
      } else {
        callback.wifiTetheringEnabledChange(null);
      }
    });
  },

  
  setUSBTethering: function setUSBTethering(enable, config, callback) {
    config.cmd = "setUSBTethering";
    
    config.isAsync = true;
    this.controlMessage(config, function setUsbTetheringResult(data) {
      let code = data.resultCode;
      let reason = data.resultReason;
      let enable = data.enable;
      let enableString = enable ? "Enable" : "Disable";

      if(DEBUG) debug(enableString + " USB tethering result: Code " + code + " reason " + reason);

      if (isError(code)) {
        callback.usbTetheringEnabledChange("netd command error");
      } else {
        callback.usbTetheringEnabledChange(null);
      }
    });
  },

  
  enableUsbRndis: function enableUsbRndis(enable, callback) {
    if(DEBUG) debug("enableUsbRndis: " + enable);

    let params = {
      cmd: "enableUsbRndis",
      enable: enable
    };
    
    if (callback) {
      params.report = true;
    } else {
      params.report = false;
    }

    
    params.isAsync = true;
    
    this.controlMessage(params, function (data) {
      callback.enableUsbRndisResult(data.result, data.enable);
    });
  },

  updateUpStream: function updateUpStream(previous, current, callback) {
    let params = {
      cmd: "updateUpStream",
      isAsync: true,
      previous: previous,
      current: current
    };

    this.controlMessage(params, function (data) {
      let code = data.resultCode;
      let reason = data.resultReason;
      if(DEBUG) debug("updateUpStream result: Code " + code + " reason " + reason);
      callback.updateUpStreamResult(!isError(code), data.current.externalIfname);
    });
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NetworkService]);
