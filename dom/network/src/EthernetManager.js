





"use strict";

const DEBUG = false;
function debug(s) {
  if (DEBUG) {
    dump("-*- EthernetManager: " + s + "\n");
  }
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const TOPIC_INTERFACE_STATE_CHANGED = "network-interface-state-changed";

const ETHERNET_NETWORK_IFACE_PREFIX = "eth";
const DEFAULT_ETHERNET_NETWORK_IFACE = "eth0";

const INTERFACE_IPADDR_NULL = "0.0.0.0";
const INTERFACE_GATEWAY_NULL = "0.0.0.0";
const INTERFACE_PREFIX_NULL = 0;
const INTERFACE_MACADDR_NULL = "00:00:00:00:00:00";

const NETWORK_INTERFACE_UP   = "up";
const NETWORK_INTERFACE_DOWN = "down";

const IP_MODE_DHCP = "dhcp";
const IP_MODE_STATIC = "static";

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");




function EthernetInterface(attr) {
  this.state = attr.state;
  this.type = attr.type;
  this.name = attr.name;
  this.ipMode = attr.ipMode;
  this.ips = [attr.ip];
  this.prefixLengths = [attr.prefixLength];
  this.gateways = [attr.gateway];
  this.dnses = attr.dnses;
  this.httpProxyHost = "";
  this.httpProxyPort = 0;
}
EthernetInterface.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsINetworkInterface]),

  updateConfig: function(config) {
    debug("Interface " + this.name + " updateConfig " + JSON.stringify(config));
    this.state = (config.state != undefined) ?
                  config.state : this.state;
    this.ips = (config.ip != undefined) ? [config.ip] : this.ips;
    this.prefixLengths = (config.prefixLength != undefined) ?
                         [config.prefixLength] : this.prefixLengths;
    this.gateways = (config.gateway != undefined) ?
                    [config.gateway] : this.gateways;
    this.dnses = (config.dnses != undefined) ? config.dnses : this.dnses;
    this.httpProxyHost = (config.httpProxyHost != undefined) ?
                          config.httpProxyHost : this.httpProxyHost;
    this.httpProxyPort = (config.httpProxyPort != undefined) ?
                          config.httpProxyPort : this.httpProxyPort;
    this.ipMode = (config.ipMode != undefined) ?
                   config.ipMode : this.ipMode;
  },

  getAddresses: function(ips, prefixLengths) {
    ips.value = this.ips.slice();
    prefixLengths.value = this.prefixLengths.slice();

    return this.ips.length;
  },

  getGateways: function(count) {
    if (count) {
      count.value = this.gateways.length;
    }
    return this.gateways.slice();
  },

  getDnses: function(count) {
    if (count) {
      count.value = this.dnses.length;
    }
    return this.dnses.slice();
  }
};














function EthernetManager() {
  debug("EthernetManager start");

  
  this.ethernetInterfaces = {};

  
  this.lastStaticConfig = {};

  Services.obs.addObserver(this, "xpcom-shutdown", false);
}

EthernetManager.prototype = {
  classID: Components.ID("a96441dd-36b3-4f7f-963b-2c032e28a039"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIEthernetManager]),

  ethernetInterfaces: null,
  lastStaticConfig: null,

  observer: function(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        debug("xpcom-shutdown");

        this._shutdown();

        Services.obs.removeObserver(this, "xpcom-shutdown");
        break;
    }
  },

  _shutdown: function() {
    debug("shuting down.");
    (function onRemove(ifnameList) {
      if (!ifnameList.length) {
        return;
      }

      let ifname = ifnameList.shift();
      this.removeInterface(ifname, { notify: onRemove.bind(this, ifnameList) });
    }).call(this, Object.keys(this.ethernetInterfaces));
  },

  get interfaceList() {
    return Object.keys(this.ethernetInterfaces);
  },

  scan: function(callback) {
    debug("scan");

    gNetworkService.getInterfaces(function(success, list) {
      let ethList = [];

      if (!success) {
        if (callback) {
          callback.notify(ethList);
        }
        return;
      }

      for (let i = 0; i < list.length; i++) {
        debug("Found interface " + list[i]);
        if (!list[i].startsWith(ETHERNET_NETWORK_IFACE_PREFIX)) {
          continue;
        }
        ethList.push(list[i]);
      }

      if (callback) {
        callback.notify(ethList);
      }
    });
  },

  addInterface: function(ifname, callback) {
    debug("addInterfaces " + ifname);

    if (!ifname || !ifname.startsWith(ETHERNET_NETWORK_IFACE_PREFIX)) {
      if (callback) {
        callback.notify(false, "Invalid interface.");
      }
      return;
    }

    if (this.ethernetInterfaces[ifname]) {
      if (callback) {
        callback.notify(true, "Interface already exists.");
      }
      return;
    }

    gNetworkService.getInterfaceConfig(ifname, function(success, result) {
      if (!success) {
        if (callback) {
          callback.notify(false, "Netd error.");
        }
        return;
      }

      
      
      if (result.macAddr == INTERFACE_MACADDR_NULL) {
        if (callback) {
          callback.notify(false, "Interface not found.");
        }
        return;
      }

      this.ethernetInterfaces[ifname] = new EthernetInterface({
        state:        result.link == NETWORK_INTERFACE_UP ?
                        Ci.nsINetworkInterface.NETWORK_STATE_DISABLED :
                        Ci.nsINetworkInterface.NETWORK_STATE_ENABLED,
        name:         ifname,
        type:         Ci.nsINetworkInterface.NETWORK_TYPE_ETHERNET,
        ip:           result.ip,
        prefixLength: result.prefix,
        ipMode:       IP_MODE_DHCP
      });

      
      gNetworkManager.registerNetworkInterface(this.ethernetInterfaces[ifname]);

      debug("Add interface " + ifname + " success with " +
            JSON.stringify(this.ethernetInterfaces[ifname]));

      if (callback) {
        callback.notify(true, "ok");
      }
    }.bind(this));
  },

  removeInterface: function(ifname, callback) {
    debug("removeInterface");

    if (!ifname || !ifname.startsWith(ETHERNET_NETWORK_IFACE_PREFIX)) {
      if (callback) {
        callback.notify(false, "Invalid interface.");
      }
      return;
    }

    if (!this.ethernetInterfaces[ifname]) {
      if (callback) {
        callback.notify(true, "Interface does not exist.");
      }
      return;
    }

    
    this.disable(ifname, { notify: function(success, message) {
      
      
      gNetworkManager.unregisterNetworkInterface(this.ethernetInterfaces[ifname]);
      delete this.ethernetInterfaces[ifname];

      debug("Remove interface " + ifname + " success.");

      if (callback) {
        callback.notify(true, "ok");
      }
    }.bind(this)});
  },

  updateInterfaceConfig: function(ifname, config, callback) {
    debug("interfaceConfigUpdate with " + ifname);

    this._ensureIfname(ifname, callback, function(iface) {
      if (!config) {
        if (callback) {
          callback.notify(false, "No config to update.");
        }
        return;
      }

      
      if (config.state) {
        delete config.state;
      }

      let currentIpMode = iface.ipMode;

      
      this.ethernetInterfaces[iface.name].updateConfig(config);

      
      
      if (iface.state != Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
        if (callback) {
          callback.notify(true, "ok");
        }
        return;
      }

      let newIpMode = this.ethernetInterfaces[iface.name].ipMode;
      if (newIpMode == IP_MODE_STATIC) {
        this._setStaticIP(iface.name, callback);
        return;
      }
      if ((currentIpMode == IP_MODE_STATIC) && (newIpMode == IP_MODE_DHCP)) {
        gNetworkService.stopDhcp(iface.name);
        
        
        this.disconnect(iface.name, { notify: function(success, message) {
          if (!success) {
            if (callback) {
              callback.notify("Disconnect failed.");
            }
            return;
          }
          this._runDhcp(iface.name, callback);
        }.bind(this) });
        return;
      }

      if (callback) {
        callback.notify(true, "ok");
      }
    }.bind(this));
  },

  enable: function(ifname, callback) {
    debug("enable with " + ifname);

    this._ensureIfname(ifname, callback, function(iface) {
      
      if (iface.state != Ci.nsINetworkInterface.NETWORK_STATE_DISABLED) {
        if (callback) {
          callback.notify(true, "already enabled.");
        }
        return;
      }

      let ips = {};
      let prefixLengths = {};
      iface.getAddresses(ips, prefixLengths);
      let config = { ifname: iface.name,
                     ip:     ips.value[0],
                     prefix: prefixLengths.value[0],
                     link:   NETWORK_INTERFACE_UP };
      gNetworkService.setInterfaceConfig(config, function(success) {
        if (!success) {
          if (callback) {
            callback.notify(false, "Netd Error.");
          }
          return;
        }

        this.ethernetInterfaces[iface.name].updateConfig({
          state: Ci.nsINetworkInterface.NETWORK_STATE_ENABLED
        });

        debug("Interface " + iface.name + " enable success.");

        if (callback) {
          callback.notify(true, "ok");
        }
      }.bind(this));
    }.bind(this));
  },

  disable: function(ifname, callback) {
    debug("disable with " + ifname);

    this._ensureIfname(ifname, callback, function(iface) {
      if (iface.state == Ci.nsINetworkInterface.NETWORK_STATE_DISABLED) {
        if (callback) {
          callback.notify(true, "Interface is already disabled.");
        }
        return;
      }

      if (iface.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
        gNetworkService.stopDhcp(iface.name);
      }

      let ips = {};
      let prefixLengths = {};
      iface.getAddresses(ips, prefixLengths);
      let config = { ifname: iface.name,
                     ip:     ips.value[0],
                     prefix: prefixLengths.value[0],
                     link:   NETWORK_INTERFACE_DOWN };
      gNetworkService.setInterfaceConfig(config, function(success) {
        if (!success) {
          if (callback) {
            callback.notify(false, "Netd Error.");
          }
          return;
        }

        this.ethernetInterfaces[iface.name].updateConfig({
          state: Ci.nsINetworkInterface.NETWORK_STATE_DISABLED
        });

        debug("Disable interface " + iface.name + " success.");

        if (callback) {
          callback.notify(true, "ok");
        }
      }.bind(this));
    }.bind(this));
  },

  connect: function(ifname, callback) {
    debug("connect wtih " + ifname);

    this._ensureIfname(ifname, callback, function(iface) {
      
      
      if (iface.state == Ci.nsINetworkInterface.NETWORK_STATE_DISABLED ||
          iface.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
        if (callback) {
          callback.notify(true, "Interface " + ifname + " is not available or "
                                 + " already connected.");
        }
        return;
      }

      if (iface.ipMode == IP_MODE_DHCP) {
        this._runDhcp(iface.name, callback);
        return;
      }

      if (iface.ipMode == IP_MODE_STATIC) {
        if (this._checkConfigNull(iface) && this.lastStaticConfig[iface.name]) {
          debug("connect with lastStaticConfig " +
                JSON.stringify(this.lastStaticConfig[iface.name]));
          this.ethernetInterfaces[iface.name].updateConfig(
            this.lastStaticConfig[iface.name]);
        }
        this._setStaticIP(iface.name, callback);
        return;
      }

      if (callback) {
        callback.notify(false, "Ip mode is wrong or not set.");
      }
    }.bind(this));
  },

  disconnect: function(ifname, callback) {
    debug("disconnect");

    this._ensureIfname(ifname, callback, function(iface) {
      
      if (iface.state != Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
        if (callback) {
          callback.notify(true, "interface is already disconnected");
        }
        return;
      }

      let config = { ifname: iface.name,
                     ip:     INTERFACE_IPADDR_NULL,
                     prefix: INTERFACE_PREFIX_NULL,
                     link:   NETWORK_INTERFACE_UP };
      gNetworkService.setInterfaceConfig(config, function(success) {
        if (!success) {
          if (callback) {
            callback.notify(false, "Netd error.");
          }
          return;
        }

        
        gNetworkService.stopDhcp(iface.name);

        this.ethernetInterfaces[iface.name].updateConfig({
          state:        Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED,
          ip:           INTERFACE_IPADDR_NULL,
          prefixLength: INTERFACE_PREFIX_NULL,
          gateway:      INTERFACE_GATEWAY_NULL
        });

        Services.obs.notifyObservers(this.ethernetInterfaces[iface.name],
                                     TOPIC_INTERFACE_STATE_CHANGED,
                                     null);

        debug("Disconnect interface " + iface.name + " success.");

        if (callback) {
          callback.notify(true, "ok");
        }
      }.bind(this));
    }.bind(this));
  },

  _checkConfigNull: function(iface) {
    let ips = {};
    let prefixLengths = {};
    let gateways = iface.getGateways();
    iface.getAddresses(ips, prefixLengths);

    if (ips.value[0] == INTERFACE_IPADDR_NULL &&
        prefixLengths.value[0] == INTERFACE_PREFIX_NULL &&
        gateways[0] == INTERFACE_GATEWAY_NULL) {
      return true;
    }

    return false;
  },

  _ensureIfname: function(ifname, callback, func) {
    
    if (!ifname) {
      ifname = DEFAULT_ETHERNET_NETWORK_IFACE;
    }

    let iface = this.ethernetInterfaces[ifname];
    if (!iface) {
      if (callback) {
        callback.notify(true, "Interface " + ifname + " is not available.");
      }
      return;
    }

    func.call(this, iface);
  },

  _runDhcp: function(ifname, callback) {
    debug("runDhcp with " + ifname);

    if (!this.ethernetInterfaces[ifname]) {
      callback.notify(false, "Invalid interface.");
      return
    }

    gNetworkService.runDhcp(ifname, function(success, result) {
      if (!success) {
        callback.notify(false, "Dhcp failed.");
        return;
      }

      debug("Dhcp success with " + JSON.stringify(result));

      
      if (this.lastStaticConfig[ifname]) {
        this.lastStaticConfig[ifname] = null;
      }

      this.ethernetInterfaces[ifname].updateConfig({
        state:        Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED,
        ip:           result.ip,
        gateway:      result.gateway,
        prefixLength: result.prefix,
        dnses:        [result.dns1, result.dns2]
      });

      Services.obs.notifyObservers(this.ethernetInterfaces[ifname],
                                   TOPIC_INTERFACE_STATE_CHANGED,
                                   null);

      debug("Connect interface " + ifname + "with dhcp success.");

      callback.notify(true, "ok");
    }.bind(this));
  },

  _setStaticIP: function(ifname, callback) {
    let iface = this.ethernetInterfaces[ifname];
    if (!iface) {
      callback.notify(false, "Invalid interface.");
      return;
    }

    let ips = {};
    let prefixLengths = {};
    iface.getAddresses(ips, prefixLengths);

    let config = { ifname: iface.name,
                   ip:     ips.value[0],
                   prefix: prefixLengths.value[0],
                   link:   NETWORK_INTERFACE_UP };
    gNetworkService.setInterfaceConfig(config, function(success) {
      if (!success) {
        callback.notify(false, "Netd Error.");
        return;
      }

      
      let ips = {};
      let prefixLengths = {};
      let gateways = iface.getGateways();
      iface.getAddresses(ips, prefixLengths);

      this.lastStaticConfig[iface.name] = {
        ip:           ips.value[0],
        prefixLength: prefixLengths.value[0],
        gateway:      gateways[0]
      };

      this.ethernetInterfaces[ifname].updateConfig({
        state: Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED,
      });

      Services.obs.notifyObservers(this.ethernetInterfaces[ifname],
                                   TOPIC_INTERFACE_STATE_CHANGED,
                                   null);

      debug("Connect interface " + ifname + "with static ip success.");

      callback.notify(true, "ok");
    }.bind(this));
  },
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([EthernetManager]);
