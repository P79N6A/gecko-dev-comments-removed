





"use strict";

this.EXPORTED_SYMBOLS = ["WifiCommand"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/systemlibs.js");

const SUPP_PROP = "init.svc.wpa_supplicant";
const WPA_SUPPLICANT = "wpa_supplicant";
const DEBUG = false;

this.WifiCommand = function(aControlMessage, aInterface, aSdkVersion) {
  function debug(msg) {
    if (DEBUG) {
      dump('-------------- WifiCommand: ' + msg);
    }
  }

  var command = {};

  
  
  
  command.getSdkVersion = function() {
    return aSdkVersion;
  };

  
  
  

  command.loadDriver = function (callback) {
    voidControlMessage("load_driver", function(status) {
      callback(status);
    });
  };

  command.unloadDriver = function (callback) {
    voidControlMessage("unload_driver", function(status) {
      callback(status);
    });
  };

  command.startSupplicant = function (callback) {
    voidControlMessage("start_supplicant", callback);
  };

  command.killSupplicant = function (callback) {
    
    
    
    
    
    stopProcess(SUPP_PROP, WPA_SUPPLICANT, callback);
  };

  command.terminateSupplicant = function (callback) {
    doBooleanCommand("TERMINATE", "OK", callback);
  };

  command.stopSupplicant = function (callback) {
    voidControlMessage("stop_supplicant", callback);
  };

  command.listNetworks = function (callback) {
    doStringCommand("LIST_NETWORKS", callback);
  };

  command.addNetwork = function (callback) {
    doIntCommand("ADD_NETWORK", callback);
  };

  command.setNetworkVariable = function (netId, name, value, callback) {
    doBooleanCommand("SET_NETWORK " + netId + " " + name + " " +
                      value, "OK", callback);
  };

  command.getNetworkVariable = function (netId, name, callback) {
    doStringCommand("GET_NETWORK " + netId + " " + name, callback);
  };

  command.removeNetwork = function (netId, callback) {
    doBooleanCommand("REMOVE_NETWORK " + netId, "OK", callback);
  };

  command.enableNetwork = function (netId, disableOthers, callback) {
    doBooleanCommand((disableOthers ? "SELECT_NETWORK " : "ENABLE_NETWORK ") +
                     netId, "OK", callback);
  };

  command.disableNetwork = function (netId, callback) {
    doBooleanCommand("DISABLE_NETWORK " + netId, "OK", callback);
  };

  command.status = function (callback) {
    doStringCommand("STATUS", callback);
  };

  command.ping = function (callback) {
    doBooleanCommand("PING", "PONG", callback);
  };

  command.scanResults = function (callback) {
    doStringCommand("SCAN_RESULTS", callback);
  };

  command.disconnect = function (callback) {
    doBooleanCommand("DISCONNECT", "OK", callback);
  };

  command.reconnect = function (callback) {
    doBooleanCommand("RECONNECT", "OK", callback);
  };

  command.reassociate = function (callback) {
    doBooleanCommand("REASSOCIATE", "OK", callback);
  };

  command.setBackgroundScan = function (enable, callback) {
    doBooleanCommand("SET pno " + (enable ? "1" : "0"),
                     "OK",
                     function(ok) {
                       callback(true, ok);
                     });
  };

  command.doSetScanMode = function (setActive, callback) {
    doBooleanCommand(setActive ?
                     "DRIVER SCAN-ACTIVE" :
                     "DRIVER SCAN-PASSIVE", "OK", callback);
  };

  command.scan = function (callback) {
    doBooleanCommand("SCAN", "OK", callback);
  };

  command.setLogLevel = function (level, callback) {
    doBooleanCommand("LOG_LEVEL " + level, "OK", callback);
  };

  command.getLogLevel = function (callback) {
    doStringCommand("LOG_LEVEL", callback);
  };

  command.wpsPbc = function (callback, iface) {
    let cmd = 'WPS_PBC';

    
    
    
    
    
    
    
    
    if (iface && isJellybean()) {
      cmd += (' inferface=' + iface);
    }

    doBooleanCommand(cmd, "OK", callback);
  };

  command.wpsPin = function (detail, callback) {
    let cmd = 'WPS_PIN ';

    
    if (detail.iface && isJellybean()) {
      cmd += ('inferface=' + iface + ' ');
    }

    cmd += (detail.bssid === undefined ? "any" : detail.bssid);
    cmd += (detail.pin === undefined ? "" : (" " + detail.pin));

    doStringCommand(cmd, callback);
  };

  command.wpsCancel = function (callback) {
    doBooleanCommand("WPS_CANCEL", "OK", callback);
  };

  command.startDriver = function (callback) {
    doBooleanCommand("DRIVER START", "OK");
  };

  command.stopDriver = function (callback) {
    doBooleanCommand("DRIVER STOP", "OK");
  };

  command.startPacketFiltering = function (callback) {
    var commandChain = ["DRIVER RXFILTER-ADD 0",
                        "DRIVER RXFILTER-ADD 1",
                        "DRIVER RXFILTER-ADD 3",
                        "DRIVER RXFILTER-START"];

    doBooleanCommandChain(commandChain, callback);
  };

  command.stopPacketFiltering = function (callback) {
    var commandChain = ["DRIVER RXFILTER-STOP",
                        "DRIVER RXFILTER-REMOVE 3",
                        "DRIVER RXFILTER-REMOVE 1",
                        "DRIVER RXFILTER-REMOVE 0"];

    doBooleanCommandChain(commandChain, callback);
  };

  command.doGetRssi = function (cmd, callback) {
    doCommand(cmd, function(data) {
      var rssi = -200;

      if (!data.status) {
        
        var reply = data.reply;
        if (reply !== "OK") {
          
          var offset = reply.lastIndexOf("rssi ");
          if (offset !== -1) {
            rssi = reply.substr(offset + 5) | 0;
          }
        }
      }
      callback(rssi);
    });
  };

  command.getRssi = function (callback) {
    command.doGetRssi("DRIVER RSSI", callback);
  };

  command.getRssiApprox = function (callback) {
    command.doGetRssi("DRIVER RSSI-APPROX", callback);
  };

  command.getLinkSpeed = function (callback) {
    doStringCommand("DRIVER LINKSPEED", function(reply) {
      if (reply) {
        reply = reply.split(" ")[1] | 0; 
      }
      callback(reply);
    });
  };

  let infoKeys = [{regexp: /RSSI=/i,      prop: 'rssi'},
                  {regexp: /LINKSPEED=/i, prop: 'linkspeed'}];

  command.getConnectionInfoICS = function (callback) {
    doStringCommand("SIGNAL_POLL", function(reply) {
      if (!reply) {
        callback(null);
        return;
      }

      
      
      
      let rval = {};
      for (let i = 0; i < infoKeys.length; i++) {
        let re = infoKeys[i].regexp;
        let iKeyStart = reply.search(re);
        if (iKeyStart !== -1) {
          let prop = infoKeys[i].prop;
          let iValueStart = reply.indexOf('=', iKeyStart) + 1;
          let iNewlineAfterValue = reply.indexOf('\n', iValueStart);
          let iValueEnd = iNewlineAfterValue !== -1
                        ? iNewlineAfterValue
                        : reply.length;
          rval[prop] = reply.substring(iValueStart, iValueEnd) | 0;
        }
      }

      callback(rval);
    });
  };

  command.getMacAddress = function (callback) {
    doStringCommand("DRIVER MACADDR", function(reply) {
      if (reply) {
        reply = reply.split(" ")[2]; 
      }
      callback(reply);
    });
  };

  command.connectToHostapd = function(callback) {
    voidControlMessage("connect_to_hostapd", callback);
  };

  command.closeHostapdConnection = function(callback) {
    voidControlMessage("close_hostapd_connection", callback);
  };

  command.hostapdCommand = function (callback, request) {
    var msg = { cmd:     "hostapd_command",
                request: request,
                iface:   aInterface };

    aControlMessage(msg, function(data) {
      callback(data.status ? null : data.reply);
    });
  };

  command.hostapdGetStations = function (callback) {
    var msg = { cmd:     "hostapd_get_stations",
                iface:   aInterface };

    aControlMessage(msg, function(data) {
      callback(data.status);
    });
  };

  command.setPowerModeICS = function (mode, callback) {
    doBooleanCommand("DRIVER POWERMODE " + (mode === "AUTO" ? 0 : 1), "OK", callback);
  };

  command.setPowerModeJB = function (mode, callback) {
    doBooleanCommand("SET ps " + (mode === "AUTO" ? 1 : 0), "OK", callback);
  };

  command.getPowerMode = function (callback) {
    doStringCommand("DRIVER GETPOWER", function(reply) {
      if (reply) {
        reply = (reply.split()[2]|0); 
      }
      callback(reply);
    });
  };

  command.setNumAllowedChannels = function (numChannels, callback) {
    doBooleanCommand("DRIVER SCAN-CHANNELS " + numChannels, "OK", callback);
  };

  command.getNumAllowedChannels = function (callback) {
    doStringCommand("DRIVER SCAN-CHANNELS", function(reply) {
      if (reply) {
        reply = (reply.split()[2]|0); 
      }
      callback(reply);
    });
  };

  command.setBluetoothCoexistenceMode = function (mode, callback) {
    doBooleanCommand("DRIVER BTCOEXMODE " + mode, "OK", callback);
  };

  command.setBluetoothCoexistenceScanMode = function (mode, callback) {
    doBooleanCommand("DRIVER BTCOEXSCAN-" + (mode ? "START" : "STOP"),
                     "OK", callback);
  };

  command.saveConfig = function (callback) {
    
    doBooleanCommand("AP_SCAN 1", "OK", function(ok) {
      doBooleanCommand("SAVE_CONFIG", "OK", callback);
    });
  };

  command.reloadConfig = function (callback) {
    doBooleanCommand("RECONFIGURE", "OK", callback);
  };

  command.setScanResultHandling = function (mode, callback) {
    doBooleanCommand("AP_SCAN " + mode, "OK", callback);
  };

  command.addToBlacklist = function (bssid, callback) {
    doBooleanCommand("BLACKLIST " + bssid, "OK", callback);
  };

  command.clearBlacklist = function (callback) {
    doBooleanCommand("BLACKLIST clear", "OK", callback);
  };

  command.setSuspendOptimizationsICS = function (enabled, callback) {
    doBooleanCommand("DRIVER SETSUSPENDOPT " + (enabled ? 0 : 1),
                     "OK", callback);
  };

  command.setSuspendOptimizationsJB = function (enabled, callback) {
    doBooleanCommand("DRIVER SETSUSPENDMODE " + (enabled ? 1 : 0),
                     "OK", callback);
  };

  command.connectToSupplicant = function(callback) {
    voidControlMessage("connect_to_supplicant", callback);
  };

  command.closeSupplicantConnection = function(callback) {
    voidControlMessage("close_supplicant_connection", callback);
  };

  command.getMacAddress = function(callback) {
    doStringCommand("DRIVER MACADDR", function(reply) {
      if (reply) {
        reply = reply.split(" ")[2]; 
      }
      callback(reply);
    });
  };

  command.setDeviceName = function(deviceName, callback) {
    doBooleanCommand("SET device_name " + deviceName, "OK", callback);
  };

  
  
  

  command.p2pProvDiscovery = function(address, wpsMethod, callback) {
    var command = "P2P_PROV_DISC " + address + " " + wpsMethod;
    doBooleanCommand(command, "OK", callback);
  };

  command.p2pConnect = function(config, callback) {
    var command = "P2P_CONNECT " + config.address + " " + config.wpsMethodWithPin + " ";
    if (config.joinExistingGroup) {
      command += "join";
    } else {
      command += "go_intent=" + config.goIntent;
    }

    debug('P2P connect command: ' + command);
    doBooleanCommand(command, "OK", callback);
  };

  command.p2pGroupRemove = function(iface, callback) {
    debug("groupRemove()");
    doBooleanCommand("P2P_GROUP_REMOVE " + iface, "OK", callback);
  };

  command.p2pEnable = function(detail, callback) {
    var commandChain = ["SET device_name "    + detail.deviceName,
                        "SET device_type "    + detail.deviceType,
                        "SET config_methods " + detail.wpsMethods,
                        "P2P_SET conc_pref sta",
                        "P2P_FLUSH"];

    doBooleanCommandChain(commandChain, callback);
  };

  command.p2pDisable = function(callback) {
    doBooleanCommand("P2P_SET disabled 1", "OK", callback);
  };

  command.p2pEnableScan = function(timeout, callback) {
    doBooleanCommand("P2P_FIND " + timeout, "OK", callback);
  };

  command.p2pDisableScan = function(callback) {
    doBooleanCommand("P2P_STOP_FIND", "OK", callback);
  };

  command.p2pGetGroupCapab = function(address, callback) {
    command.p2pPeer(address, function(reply) {
      debug('p2p_peer reply: ' + reply);
      if (!reply) {
        callback(0);
        return;
      }
      var capab = /group_capab=0x([0-9a-fA-F]+)/.exec(reply)[1];
      if (!capab) {
        callback(0);
      } else {
        callback(parseInt(capab, 16));
      }
    });
  };

  command.p2pPeer = function(address, callback) {
    doStringCommand("P2P_PEER " + address, callback);
  };

  command.p2pGroupAdd = function(netId, callback) {
    doBooleanCommand("P2P_GROUP_ADD persistent=" + netId, callback);
  };

  command.p2pReinvoke = function(netId, address, callback) {
    doBooleanCommand("P2P_INVITE persistent=" + netId + " peer=" + address, "OK", callback);
  };

  
  
  

  function voidControlMessage(cmd, callback) {
    aControlMessage({ cmd: cmd, iface: aInterface }, function (data) {
      callback(data.status);
    });
  }

  function doCommand(request, callback) {
    var msg = { cmd:     "command",
                request: request,
                iface:   aInterface };

    aControlMessage(msg, callback);
  }

  function doIntCommand(request, callback) {
    doCommand(request, function(data) {
      callback(data.status ? -1 : (data.reply|0));
    });
  }

  function doBooleanCommand(request, expected, callback) {
    doCommand(request, function(data) {
      callback(data.status ? false : (data.reply === expected));
    });
  }

  function doStringCommand(request, callback) {
    doCommand(request, function(data) {
      callback(data.status ? null : data.reply);
    });
  }

  function doBooleanCommandChain(commandChain, callback, i) {
    if (undefined === i) {
      i = 0;
    }

    doBooleanCommand(commandChain[i], "OK", function(ok) {
      if (!ok) {
        return callback(false);
      }
      i++;
      if (i === commandChain.length || !commandChain[i]) {
        
        return callback(true);
      }
      doBooleanCommandChain(commandChain, callback, i);
    });
  }

  
  
  

  function stopProcess(service, process, callback) {
    var count = 0;
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    function tick() {
      let result = libcutils.property_get(service);
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
    }

    setProperty("ctl.stop", process, tick);
  }

  
  
  
  function setProperty(key, value, callback) {
    let ok = true;
    try {
      libcutils.property_set(key, value);
    } catch(e) {
      ok = false;
    }
    callback(ok);
  }

  function isJellybean() {
    
    
    
    
    
    
    
    
    return aSdkVersion === 16 || aSdkVersion === 17 || aSdkVersion === 18;
  }

  return command;
};
