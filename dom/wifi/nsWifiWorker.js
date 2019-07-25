







































"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const DEBUG = true; 

const WIFIWORKER_CONTRACTID = "@mozilla.org/wifi/worker;1";
const WIFIWORKER_CID        = Components.ID("{a14e8977-d259-433a-a88d-58dd44657e5b}");

const WIFIWORKER_WORKER     = "resource://gre/modules/network_worker.js";








var WifiManager = (function() {
  var controlWorker = new ChromeWorker(WIFIWORKER_WORKER);
  var eventWorker = new ChromeWorker(WIFIWORKER_WORKER);

  
  var controlCallbacks = Object.create(null);
  var idgen = 0;

  function controlMessage(obj, callback) {
    var id = idgen++;
    obj.id = id;
    if (callback)
      controlCallbacks[id] = callback;
    controlWorker.postMessage(obj);
  }

  function onerror(e) {
    
    
    
    
    
    
    
    
    e.preventDefault();

    var worker = (this === controlWorker) ? "control" : "event";

    debug("Got an error from the " + worker + " worker: " + e.filename +
          ":" + e.lineno + ": " + e.message + "\n");
  }

  controlWorker.onerror = onerror;
  eventWorker.onerror = onerror;

  controlWorker.onmessage = function(e) {
    var data = e.data;
    var id = data.id;
    var callback = controlCallbacks[id];
    if (callback) {
      callback(data);
      delete controlCallbacks[id];
    }
  };

  
  var recvErrors = 0;
  eventWorker.onmessage = function(e) {
    
    if (handleEvent(e.data.event))
      waitForEvent();
  };

  function waitForEvent() {
    eventWorker.postMessage({ cmd: "wait_for_event" });
  }

  

  function voidControlMessage(cmd, callback) {
    controlMessage({ cmd: cmd }, function (data) {
      callback(data.status);
    });
  }

  function loadDriver(callback) {
    voidControlMessage("load_driver", callback);
  }

  function unloadDriver(callback) {
    voidControlMessage("unload_driver", callback);
  }

  function startSupplicant(callback) {
    voidControlMessage("start_supplicant", callback);
  }

  function stopSupplicant(callback) {
    voidControlMessage("stop_supplicant", callback);
  }

  function connectToSupplicant(callback) {
    voidControlMessage("connect_to_supplicant", callback);
  }

  function closeSupplicantConnection(callback) {
    voidControlMessage("close_supplicant_connection", callback);
  }

  function doCommand(request, callback) {
    controlMessage({ cmd: "command", request: request }, callback);
  }

  function doIntCommand(request, callback) {
    doCommand(request, function(data) {
      callback(data.status ? -1 : (data.reply|0));
    });
  }

  function doBooleanCommand(request, expected, callback) {
    doCommand(request, function(data) {
      callback(data.status ? false : (data.reply == expected));
    });
  }

  function doStringCommand(request, callback) {
    doCommand(request, function(data) {
      callback(data.status ? null : data.reply);
    });
  }

  function listNetworksCommand(callback) {
    doStringCommand("LIST_NETWORKS", callback);
  }

  function addNetworkCommand(callback) {
    doIntCommand("ADD_NETWORK", callback);
  }

  function setNetworkVariableCommand(netId, name, value, callback) {
    doBooleanCommand("SET_NETWORK " + netId + " " + name + " " + value, "OK", callback);
  }

  function getNetworkVariableCommand(netId, name, callback) {
    doStringCommand("GET_NETWORK " + netId + " " + name, callback);
  }

  function removeNetworkCommand(netId, callback) {
    doBooleanCommand("REMOVE_NETWORK " + netId, callback);
  }

  function enableNetworkCommand(netId, disableOthers, callback) {
    doBooleanCommand((disableOthers ? "SELECT_NETWORK " : "ENABLE_NETWORK ") + netId, "OK", callback);
  }

  function disableNetworkCommand(netId, callback) {
    doBooleanCommand("DISABLE_NETWORK " + netId, "OK", callback);
  }

  function statusCommand(callback) {
    doStringCommand("STATUS", callback);
  }

  function pingCommand(callback) {
    doBooleanCommand("PING", "PONG", callback);
  }

  function scanResultsCommand(callback) {
    doStringCommand("SCAN_RESULTS", callback);
  }

  function disconnectCommand(callback) {
    doBooleanCommand("DISCONNECT", "OK", callback);
  }

  function reconnectCommand(callback) {
    doBooleanCommand("RECONNECT", "OK", callback);
  }

  function reassociateCommand(callback) {
    doBooleanCommand("REASSOCIATE", "OK", callback);
  }

  var scanModeActive = false;

  function doSetScanModeCommand(setActive, callback) {
    doBooleanCommand(setActive ? "DRIVER SCAN-ACTIVE" : "DRIVER SCAN-PASSIVE", "OK", callback);
  }

  function scanCommand(forceActive, callback) {
    if (forceActive && !scanModeActive) {
      doSetScanModeCommand(true, function(ok) {
        ok && doBooleanCommand("SCAN", "OK", function(ok) {
          ok && doSetScanModeCommand(false, callback);
        });
      });
      return;
    }
    doBooleanCommand("SCAN", "OK", callback);
  }

  function setScanModeCommand(setActive, callback) {
    scanModeActive = setActive;
    doSetScanModeCommand(setActive, callback);
  }

  function startDriverCommand(callback) {
    doBooleanCommand("DRIVER START", "OK");
  }

  function stopDriverCommand(callback) {
    doBooleanCommand("DRIVER STOP", "OK");
  }

  function startPacketFiltering(callback) {
    doBooleanCommand("DRIVER RXFILTER-ADD 0", "OK", function(ok) {
      ok && doBooleanCommand("DRIVER RXFILTER-ADD 1", "OK", function(ok) {
        ok && doBooleanCommand("DRIVER RXFILTER-ADD 3", "OK", function(ok) {
          ok && doBooleanCommand("DRIVER RXFILTER-START", "OK", callback)
        });
      });
    });
  }

  function stopPacketFiltering(callback) {
    doBooleanCommand("DRIVER RXFILTER-STOP", "OK", function(ok) {
      ok && doBooleanCommand("DRIVER RXFILTER-REMOVE 3", "OK", function(ok) {
        ok && doBooleanCommand("DRIVER RXFILTER-REMOVE 1", "OK", function(ok) {
          ok && doBooleanCommand("DRIVER RXFILTER-REMOVE 0", "OK", callback)
        });
      });
    });
  }

  function doGetRssiCommand(cmd, callback) {
    doCommand(cmd, function(data) {
      var rssi = -200;

      if (!data.status) {
        
        var reply = data.reply;
        if (reply != "OK") {
          
          var offset = reply.lastIndexOf("rssi ");
          if (offset != -1)
            rssi = reply.substr(offset + 5) | 0;
        }
      }
      callback(rssi);
    });
  }

  function getRssiCommand(callback) {
    doGetRssiCommand("DRIVER RSSI", callback);
  }

  function getRssiApproxCommand(callback) {
    doGetRssiCommand("DRIVER RSSI-APPROX", callback);
  }

  function getLinkSpeedCommand(callback) {
    doStringCommand("DRIVER LINKSPEED", function(reply) {
      if (reply)
        reply = reply.split()[1] | 0; 
      callback(reply);
    });
  }

  function getMacAddressCommand(callback) {
    doStringCommand("DRIVER MACADDR", function(reply) {
      if (reply)
        reply = reply.split(" ")[2]; 
      callback(reply);
    });
  }

  function setPowerModeCommand(mode, callback) {
    doBooleanCommand("DRIVER POWERMODE " + mode, "OK", callback);
  }

  function getPowerModeCommand(callback) {
    doStringCommand("DRIVER GETPOWER", function(reply) {
      if (reply)
        reply = (reply.split()[2]|0); 
      callback(reply);
    });
  }

  function setNumAllowedChannelsCommand(numChannels, callback) {
    doBooleanCommand("DRIVER SCAN-CHANNELS " + numChannels, "OK", callback);
  }

  function getNumAllowedChannelsCommand(callback) {
    doStringCommand("DRIVER SCAN-CHANNELS", function(reply) {
      if (reply)
        reply = (reply.split()[2]|0); 
      callback(reply);
    });
  }

  function setBluetoothCoexistenceModeCommand(mode, callback) {
    doBooleanCommand("DRIVER BTCOEXMODE " + mode, "OK", callback);
  }

  function setBluetoothCoexistenceScanModeCommand(mode, callback) {
    doBooleanCommand("DRIVER BTCOEXSCAN-" + (mode ? "START" : "STOP"), "OK", callback);
  }

  function saveConfigCommand(callback) {
    
    doBooleanCommand("AP_SCAN 1", "OK", function(ok) {
      doBooleanCommand("SAVE_CONFIG", "OK", callback);
    });
  }

  function reloadConfigCommand(callback) {
    doBooleanCommand("RECONFIGURE", "OK", callback);
  }

  function setScanResultHandlingCommand(mode, callback) {
    doBooleanCommand("AP_SCAN " + mode, "OK", callback);
  }

  function addToBlacklistCommand(bssid, callback) {
    doBooleanCommand("BLACKLIST " + bssid, "OK", callback);
  }

  function clearBlacklistCommand(callback) {
    doBooleanCommand("BLACKLIST clear", "OK", callback);
  }

  function setSuspendOptimizationsCommand(enabled, callback) {
    doBooleanCommand("DRIVER SETSUSPENDOPT " + (enabled ? 0 : 1), "OK", callback);
  }

  function getProperty(key, defaultValue, callback) {
    controlMessage({ cmd: "property_get", key: key, defaultValue: defaultValue }, function(data) {
      callback(data.status < 0 ? null : data.value);
    });
  }

  function setProperty(key, value, callback) {
    controlMessage({ cmd: "property_set", key: key, value: value }, function(data) {
      callback(!data.status);
    });
  }

  function enableInterface(ifname, callback) {
    controlMessage({ cmd: "ifc_enable", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function disableInterface(ifname, callback) {
    controlMessage({ cmd: "ifc_disable", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function addHostRoute(ifname, route, callback) {
    controlMessage({ cmd: "ifc_add_host_route", ifname: ifname, route: route }, function(data) {
      callback(!data.status);
    });
  }

  function removeHostRoutes(ifname, callback) {
    controlMessage({ cmd: "ifc_remove_host_routes", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function setDefaultRoute(ifname, route, callback) {
    controlMessage({ cmd: "ifc_set_default_route", ifname: ifname, route: route }, function(data) {
      callback(!data.status);
    });
  }

  function getDefaultRoute(ifname, callback) {
    controlMessage({ cmd: "ifc_get_default_route", ifname: ifname }, function(data) {
      callback(!data.route);
    });
  }

  function removeDefaultRoute(ifname, callback) {
    controlMessage({ cmd: "ifc_remove_default_route", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function resetConnections(ifname, callback) {
    controlMessage({ cmd: "ifc_reset_connections", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function runDhcp(ifname, callback) {
    controlMessage({ cmd: "dhcp_do_request", ifname: ifname }, function(data) {
      callback(data.status ? null : data);
    });
  }

  function stopDhcp(ifname, callback) {
    controlMessage({ cmd: "dhcp_stop", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function releaseDhcpLease(ifname, callback) {
    controlMessage({ cmd: "dhcp_release_lease", ifname: ifname }, function(data) {
      callback(!data.status);
    });
  }

  function getDhcpError(callback) {
    controlMessage({ cmd: "dhcp_get_errmsg" }, function(data) {
      callback(data.error);
    });
  }

  function configureInterface(ifname, ipaddr, mask, gateway, dns1, dns2, callback) {
    controlMessage({ cmd: "ifc_configure", ifname: ifname,
                     ipaddr: ipaddr, mask: mask, gateway: gateway,
                     dns1: dns1, dns2: dns2}, function(data) {
      callback(!data.status);
    });
  }

  function runDhcpRenew(ifname, callback) {
    controlMessage({ cmd: "dhcp_do_request", ifname: ifname }, function(data) {
      callback(data.status ? null : data);
    });
  }

  var manager = {};

  function notify(eventName, eventObject) {
    var handler = manager["on" + eventName];
    if (handler) {
      if (!eventObject)
        eventObject = ({});
      handler.call(eventObject);
    }
  }

  function notifyStateChange(fields) {
    fields.prevState = manager.state;
    manager.state = fields.state;
    notify("statechange", fields);
  }

  function parseStatus(status, reconnected) {
    if (status === null) {
      debug("Unable to get wpa supplicant's status");
      return;
    }

    var lines = status.split("\n");
    for (let i = 0; i < lines.length; ++i) {
      let [key, value] = lines[i].split("=");
      if (key === "wpa_state") {
        notifyStateChange({ state: value });
        if (value === "COMPLETED")
          onconnected(reconnected);
      }
    }
  }

  
  var connectTries = 0;
  var retryTimer = null;
  function connectCallback(ok) {
    if (ok === 0) {
      
      retryTimer = null;
      didConnectSupplicant(false, function(){});
      return;
    }
    if (connectTries++ < 3) {
      
      if (!retryTimer)
        retryTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

      retryTimer.initWithCallback(function(timer) {
          connectToSupplicant(connectCallback);
        }, 5000, Ci.nsITimer.TYPE_ONE_SHOT);
      return;
    }

    retryTimer = null;
    notify("supplicantlost");
  }

  manager.start = function() {
    
    
    
    if (manager.state === "UNINITIALIZED")
      connectToSupplicant(connectCallback);
  }

  function dhcpAfterConnect() {
    runDhcp(manager.ifname, function (data) {
      if (!data) {
        debug("DHCP failed to run");
        return;
      }
      setProperty("net.dns1", ipToString(data.dns1), function(ok) {
        if (!ok) {
          debug("Unable to set net.dns1");
          return;
        }
        setProperty("net.dns2", ipToString(data.dns2), function(ok) {
          if (!ok) {
            debug("Unable to set net.dns2");
            return;
          }
          getProperty("net.dnschange", "0", function(value) {
            if (value === null) {
              debug("Unable to get net.dnschange");
              return;
            }
            setProperty("net.dnschange", String(Number(value) + 1), function(ok) {
              if (!ok)
                debug("Unable to set net.dnschange");
            });
          });
        });
      });
    });
  }

  function onconnected(reconnected) {
    if (!reconnected) {
      dhcpAfterConnect();
      return;
    }

    
    
    getProperty("init.svc.dhcpcd_" + manager.ifname, "stopped", function(value) {
      if (value === "running") {
        return;
      }

      
      getProperty("init.svc.dhcpcd", "stopped", function(value) {
        if (value === "running") {
          return;
        }

        dhcpAfterConnect();
      });
    });
  }

  var supplicantStatesMap = ["DISCONNECTED", "INACTIVE", "SCANNING", "ASSOCIATING",
                             "ASSOCIATED", "FOUR_WAY_HANDSHAKE", "GROUP_HANDSHAKE",
                             "COMPLETED", "DORMANT", "UNINITIALIZED"];
  var driverEventMap = { STOPPED: "driverstopped", STARTED: "driverstarted", HANGED: "driverhung" };

  
  function handleEvent(event) {
    debug("Event coming in: " + event);
    if (event.indexOf("CTRL-EVENT-") !== 0) {
      debug("Got weird event, possibly not doing anything.");
      if (event.indexOf("WPA:") == 0 &&
          event.indexOf("pre-shared key may be incorrect") != -1) {
        notify("passwordmaybeincorrect");
      }
      return true;
    }

    var space = event.indexOf(" ");
    var eventData = event.substr(0, space + 1);
    if (eventData.indexOf("CTRL-EVENT-STATE-CHANGE") === 0) {
      
      var fields = {};
      var tokens = event.substr(space + 1).split(" ");
      for (var n = 0; n < tokens.length; ++n) {
        var kv = tokens[n].split("=");
        if (kv.length === 2)
          fields[kv[0]] = kv[1];
      }
      if (!("state" in fields))
        return true;
      fields.state = supplicantStatesMap[fields.state];
      notifyStateChange(fields);
      return true;
    }
    if (eventData.indexOf("CTRL-EVENT-DRIVER-STATE") === 0) {
      var handlerName = driverEventMap[eventData];
      if (handlerName)
        notify(handlerName);
      return true;
    }
    if (eventData.indexOf("CTRL-EVENT-TERMINATING") === 0) {
      
      
      
      if (eventData.indexOf("connection closed") !== -1)
        return false;

      
      
      if (eventData.indexOf("recv error") !== -1 && ++recvErrors < 10)
        return true;

      notify("supplicantlost");
      return false;
    }
    if (eventData.indexOf("CTRL-EVENT-DISCONNECTED") === 0) {
      notifyStateChange({ state: "DISCONNECTED" });
      return true;
    }
    if (eventData.indexOf("CTRL-EVENT-CONNECTED") === 0) {
      
      var bssid = eventData.split(" ")[4];
      var id = eventData.substr(eventData.indexOf("id=")).split(" ")[0];
      notifyStateChange({ state: "CONNECTED", BSSID: bssid, id: id });
      onconnected(false);
      return true;
    }
    if (eventData.indexOf("CTRL-EVENT-SCAN-RESULTS") === 0) {
      debug("Notifying of scan results available");
      notify("scanresultsavailable");
      return true;
    }
    
    return true;
  }

  const SUPP_PROP = "init.svc.wpa_supplicant";
  function killSupplicant(callback) {
    
    
    
    
    
    var count = 0;
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    function tick() {
      getProperty(SUPP_PROP, "stopped", function (result) {
        if (result === null) {
          callback();
          return;
        }
        if (result === "stopped" || ++count >= 5) {
          
          timer = null;
          callback();
          return;
        }

        
        timer.initWithCallback(tick, 1000, Ci.nsITimer.TYPE_ONE_SHOT);
      });
    }

    setProperty("ctl.stop", "wpa_supplicant", tick);
  }

  function didConnectSupplicant(reconnected, callback) {
    waitForEvent();
    notify("supplicantconnection");

    
    statusCommand(function(status) {
      parseStatus(status, reconnected);
      callback();
    });
  }

  function prepareForStartup(callback) {
    
    
    getProperty(SUPP_PROP, "stopped", function (value) {
      debug(SUPP_PROP + " was " + value);
      if (value !== "running") {
        stopDhcp(manager.ifname, function() { callback(false) });
        return;
      }

      debug(SUPP_PROP + " was running, trying to connect");
      
      connectToSupplicant(function (retval) {
        if (retval === 0) {
          
          debug("Successfully connected!");

          
          
          
          didConnectSupplicant(true, function() { callback(true) });
          return;
        }

        debug("Didn't connect, trying other method.");
        stopDhcp(manager.ifname, function() {
          
          killSupplicant(function() { callback(false); });
        });
      });
    });
  }

  
  var airplaneMode = false;
  manager.state = "UNINITIALIZED";

  
  manager.setWifiEnabled = function(enable, callback) {
    var targetState = enable ? "ENABLED" : "DISABLED";
    if (enable == targetState)
      return true;
    if (enable && airplaneMode)
      return false;
    if (enable) {
      
      getProperty("wifi.interface", "tiwlan0", function (ifname) {
        if (!ifname) {
          callback(-1);
          return;
        }
        manager.ifname = ifname;

        prepareForStartup(function(already_connected) {
          if (already_connected) {
            callback(0);
            return;
          }

          loadDriver(function (status) {
            if (status < 0) {
              callback(status);
              return;
            }
            startSupplicant(function (status) {
              if (status < 0) {
                callback(status);
                return;
              }
              enableInterface(ifname, function (ok) {
                callback(ok ? 0 : -1);
              });
            });
          });
        });
      });
    } else {
      stopSupplicant(function (status) {
        if (ok < 0) {
          callback(-1);
          return;
        }
        disableInterface(manager.ifname, function (ok) {
          unloadDriver(callback);
        });
      });
    }
  }

  manager.disconnect = disconnectCommand;
  manager.reconnect = reconnectCommand;
  manager.reassociate = reassociateCommand;

  var networkConfigurationFields = ["ssid", "bssid", "psk", "wep_key0", "wep_key1", "wep_key2", "wep_key3",
                                    "wep_tx_keyidx", "priority", "key_mgmt", "scan_ssid", "disabled"];

  manager.getNetworkConfiguration = function(config, callback) {
    var netId = config.netId;
    var done = 0;
    for (var n = 0; n < networkConfigurationFields.length; ++n) {
      var fieldName = networkConfigurationFields[n];
      getNetworkVariableCommand(netId, fieldName, function(value) {
        config[fieldName] = value;
        if (++done == networkConfigurationFields.length)
          callback(config);
      });
    }
  }
  manager.setNetworkConfiguration = function(config, callback) {
    var netId = config.netId;
    var done = 0;
    var errors = 0;
    for (var n = 0; n < networkConfigurationFields.length; ++n) {
      var fieldName = networkConfigurationFields[n];
      if (!(fieldName in config)) {
        ++done;
      } else {
        setNetworkVariableCommand(netId, fieldName, config[fieldName], function(ok) {
          if (!ok)
            ++errors;
          if (++done == networkConfigurationFields.length)
            callback(errors == 0);
        });
      }
    }
    
    if (done == networkConfigurationFields.length)
      callback(false);
  }
  manager.getConfiguredNetworks = function(callback) {
    listNetworksCommand(function (reply) {
      var networks = {};
      var done = 0;
      var errors = 0;
      var lines = reply.split("\n");
      for (var n = 1; n < lines.length; ++n) {
        var result = lines[n].split("\t");
        var netId = result[0];
        var config = networks[netId] = { netId: netId };
        switch (result[3]) {
        case "[CURRENT]":
          config.status = "CURRENT";
          break;
        case "[DISABLED]":
          config.status = "DISABLED";
          break;
        default:
          config.status = "ENABLED";
          break;
        }
        manager.getNetworkConfiguration(config, function (ok) {
            if (!ok)
              ++errors;
            if (++done == lines.length - 1) {
              if (errors) {
                
                removeNetworkCommand(netId, function() {
                  callback(null);
                });
              } else {
                callback(networks);
              }
            }
        });
      }
    });
  }
  manager.addNetwork = function(config, callback) {
    addNetworkCommand(function (netId) {
      config.netId = netId;
      manager.setNetworkConfiguration(config, callback);
    });
  }
  manager.updateNetwork = function(config, callback) {
    manager.setNetworkConfiguration(config, callback);
  }
  manager.removeNetwork = function(netId, callback) {
    removeNetworkCommand(netId, callback);
  }

  function ipToString(n) {
    return String((n >>  0) & 0xFF) + "." +
                 ((n >>  8) & 0xFF) + "." +
                 ((n >> 16) & 0xFF) + "." +
                 ((n >> 24) & 0xFF);
  }

  manager.enableNetwork = function(netId, disableOthers, callback) {
    enableNetworkCommand(netId, disableOthers, callback);
  }
  manager.disableNetwork = function(netId, callback) {
    disableNetworkCommand(netId, callback);
  }
  manager.getMacAddress = getMacAddressCommand;
  manager.getScanResults = scanResultsCommand;
  manager.setScanMode = function(mode, callback) {
    setScanModeCommand(mode === "active", callback);
  }
  manager.scan = scanCommand;
  return manager;
})();

function WifiNetwork(ssid, bssid, flags, signal) {
  this.ssid = ssid;
  this.bssid = bssid;
  this.flags = flags;
  this.signal = Number(signal);
}

WifiNetwork.prototype.QueryInterface = XPCOMUtils.generateQI([Ci.nsIWifiNetwork]);

function nsWifiWorker() {
  WifiManager.onsupplicantconnection = function() {
    debug("Connected to supplicant");
    WifiManager.getMacAddress(function (mac) {
      debug("Got mac: " + mac);
    });
    connectToMozilla();
  }
  WifiManager.onsupplicantlost = function() {
    debug("Couldn't connect to supplicant");
  }

  var self = this;

  this.networks = Object.create(null);
  WifiManager.onstatechange = function() {
    debug("State change: " + this.prevState + " -> " + this.state);
  }

  function connectToMozilla() {
    
    
    

    debug("Haven't connected to a network, trying a default (for now)");
    var configs = [
      { "ssid": '"mozilla demo"', "key_mgmt": "NONE", "scan_ssid": 1, "disabled": 0 },
      { "ssid": '"Mozilla"', "key_mgmt": "NONE", "disabled": 0 },
      { "ssid": '"Mozilla Guest"', "key_mgmt": "NONE", "scan_ssid": 1, "disabled": 0 },
    ];

    var i = 0;
    function addThem() {
      WifiManager.addNetwork(configs[i++], function(ok) {
        if (!ok) {
          debug("Unable to add the network!");
          return;
        }

        if (i < configs.length) {
          addThem();
          return;
        }

        
        
        WifiManager.scan(false, function(){});
      });
    }
    addThem();
  }

  WifiManager.onscanresultsavailable = function() {
    debug("Scan results are available! Asking for them.");
    WifiManager.getScanResults(function(r) {
      
      
      WifiManager.setScanMode("inactive", function() {});
      let lines = r.split("\n");
      
      for (let i = 1; i < lines.length; ++i) {
        
        var match = /([\S]+)\s+([\S]+)\s+([\S]+)\s+(\[[\S]+\])?\s+(.*)/.exec(lines[i]);

        
        if (match && match[5])
          self.networks[match[5]] = new WifiNetwork(match[5], match[1], match[4], match[3]);
        else if (!match)
          debug("Match didn't find anything for: " + lines[i]);
      }

      if (self.wantScanResults) {
        self.wantScanResults();
      }
    });
  }

  WifiManager.setWifiEnabled(true, function (ok) {
      if (ok === 0)
        WifiManager.start();
      else
        debug("Couldn't start Wifi");
    });

  debug("Wifi starting");
}

nsWifiWorker.prototype = {
  classID:   WIFIWORKER_CID,
  classInfo: XPCOMUtils.generateCI({classID: WIFIWORKER_CID,
                                    contractID: WIFIWORKER_CONTRACTID,
                                    classDescription: "WifiWorker",
                                    interfaces: [Ci.nsIWorkerHolder,
                                                 Ci.nsIWifi]}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWorkerHolder,
                                         Ci.nsIWifi]),

  
  waitForScan: function(callback) {
    if (this.wantScanResults) {
      var older = this.wantScanResults;
      this.wantScanResults = function() { callback(); older(); };
    } else {
      this.wantScanResults = callback;
    }
  },

  

  setWifiEnabled: function(enable) {
    WifiManager.setWifiEnabled(enable, function (ok) {
      debug(ok);
    });
  },

  
  
  get worker() { throw "Not implemented"; },

  shutdown: function() {
    this.setWifiEnabled(false);
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([nsWifiWorker]);

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- nsWifiWorker component: " + s + "\n");
  };
} else {
  debug = function (s) {};
}
