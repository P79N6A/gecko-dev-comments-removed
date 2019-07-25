














"use strict";

const DEBUG = false;

const USB_FUNCTION_PATH  = "sys.usb.config";
const USB_FUNCTION_STATE = "sys.usb.state";
const USB_FUNCTION_RNDIS = "rndis,adb";
const USB_FUNCTION_ADB   = "adb";

const USB_FUNCTION_RETRY_TIMES = 20;

const USB_FUNCTION_RETRY_INTERVAL = 100;


const NETD_COMMAND_OKAY  = 200;
const NETD_COMMAND_ERROR = 300;

importScripts("systemlibs.js");

let gWifiFailChain = [stopSoftAP,
                      setIpForwardingEnabled,
                      stopTethering];

function wifiTetheringFail(params) {
  let unload = false;

  
  if (params.enable) {
    unload = true;
  }
  params.unload = unload;
  
  postMessage(params);

  
  

  
  params.enable = false;
  chain(params, gWifiFailChain, null);
}

function wifiTetheringSuccess(params) {
  let unload = false;

  
  if (!params.enable) {
    unload = true;
  }

  params.unload = unload;
  
  postMessage(params);
  return true;
}

let gUSBFailChain = [stopSoftAP,
                     setIpForwardingEnabled,
                     stopTethering];

function usbTetheringFail(params) {
  
  postMessage(params);
  
  
  let functionChain = [setIpForwardingEnabled,
                       stopTethering];

  
  params.enable = false;
  chain(params, gUSBFailChain, null);

  
  setUSBFunction({usbfunc: USB_FUNCTION_ADB, report: false});
}

function usbTetheringSuccess(params) {
  
  postMessage(params);
  return true;
}







function getIFProperties(ifname) {
  let gateway_str = libcutils.property_get("net." + ifname + ".gw");
  return {
    ifname:      ifname,
    gateway:     netHelpers.stringToIP(gateway_str),
    gateway_str: gateway_str,
    dns1_str:    libcutils.property_get("net." + ifname + ".dns1"),
    dns2_str:    libcutils.property_get("net." + ifname + ".dns2"),
  };
}








self.onmessage = function onmessage(event) {
  let message = event.data;
  if (DEBUG) debug("received message: " + JSON.stringify(message));
  
  
  let ret = self[message.cmd](message);
  if (!message.isAsync) {
    postMessage({id: message.id, ret: ret});
  }
};




function setDefaultRouteAndDNS(options) {
  if (options.oldIfname) {
    libnetutils.ifc_remove_default_route(options.oldIfname);
  }

  if (!options.gateway || !options.dns1_str) {
    options = getIFProperties(options.ifname);
  }

  libnetutils.ifc_set_default_route(options.ifname, options.gateway);
  libcutils.property_set("net.dns1", options.dns1_str);
  libcutils.property_set("net.dns2", options.dns2_str);

  
  let dnschange = libcutils.property_get("net.dnschange", "0");
  libcutils.property_set("net.dnschange", (parseInt(dnschange, 10) + 1).toString());
}





function runDHCPAndSetDefaultRouteAndDNS(options) {
  let dhcp = libnetutils.dhcp_do_request(options.ifname);
  dhcp.ifname = options.ifname;
  dhcp.oldIfname = options.oldIfname;

  
  
  
  setDefaultRouteAndDNS(dhcp);
}




function removeDefaultRoute(options) {
  libnetutils.ifc_remove_default_route(options.ifname);
}




function addHostRoute(options) {
  libnetutils.ifc_add_route(options.ifname, options.dns1, 32, options.gateway);
  libnetutils.ifc_add_route(options.ifname, options.dns2, 32, options.gateway);
  libnetutils.ifc_add_route(options.ifname, options.httpproxy, 32, options.gateway);
  libnetutils.ifc_add_route(options.ifname, options.mmsproxy, 32, options.gateway);
}




function removeHostRoute(options) {
  libnetutils.ifc_remove_route(options.ifname, options.dns1, 32, options.gateway);
  libnetutils.ifc_remove_route(options.ifname, options.dns2, 32, options.gateway);
  libnetutils.ifc_remove_route(options.ifname, options.httpproxy, 32, options.gateway);
  libnetutils.ifc_remove_route(options.ifname, options.mmsproxy, 32, options.gateway);
}

let gCommandQueue = [];
let gCurrentCommand = null;
let gCurrentCallback = null;
let gPending = false;




function onNetdMessage(data) {
  let result = "";
  let reason = "";

  
  let i = 0;
  while (i < data.length) {
    let octet = data[i];
    i += 1;
    if (octet == 32) {
      break;
    }
    result += String.fromCharCode(octet);
  }

  let code = parseInt(result);

  for (; i < data.length; i++) {
    let octet = data[i];
    reason += String.fromCharCode(octet);
  }

  
  gPending = false;
  debug("Receiving '" + gCurrentCommand + "' command response from netd.");
  debug("          ==> Code: " + code + "  Reason: " + reason);
  if (gCurrentCallback) {
    let error;
    (code >= NETD_COMMAND_OKAY && code < NETD_COMMAND_ERROR) ? (error = false) : (error = true);
    gCurrentCallback(error, {code: code, reason: reason});
  }

  
  nextNetdCommand();
}




function doCommand(command, callback) {
  debug("Preparing to send '" + command + "' command...");
  gCommandQueue.push([command, callback]);
  return nextNetdCommand();
}

function nextNetdCommand() {
  if (!gCommandQueue.length || gPending) {
    return true;
  }
  [gCurrentCommand, gCurrentCallback] = gCommandQueue.shift();
  debug("Sending '" + gCurrentCommand + "' command to netd.");
  gPending = true;
  return postNetdCommand(gCurrentCommand);
}

function setInterfaceUp(params, callback) {
  let command = "interface setcfg " + params.ifname + " " + params.ip + " " +
                params.prefix + " " + "[" + params.link + "]";
  return doCommand(command, callback);
}

function setInterfaceDown(params, callback) {
  let command = "interface setcfg " + params.ifname + " " + params.ip + " " +
                params.prefix + " " + "[" + params.link + "]";
  return doCommand(command, callback);
}

function setIpForwardingEnabled(params, callback) {
  let command;

  if (params.enable) {
    command = "ipfwd enable";
  } else {
    command = "ipfwd disable";
  }
  return doCommand(command, callback);
}

function startTethering(params, callback) {
  let command = "tether start " + params.startIp + " " + params.endIp;
  return doCommand(command, callback);
}

function stopTethering(params, callback) {
  let command = "tether stop";
  return doCommand(command, callback);
}

function tetherInterface(params, callback) {
  let command = "tether interface add " + params.ifname;
  return doCommand(command, callback);
}

function untetherInterface(params, callback) {
  let command = "tether interface remove " + params.ifname;
  return doCommand(command, callback);
}

function setDnsForwarders(params, callback) {
  let command = "tether dns set " + params.dns1 + " " + params.dns2;
  return doCommand(command, callback);
}

function enableNat(params, callback) {
  let command = "nat enable " + params.internalIfname + " " +
                params.externalIfname + " " + "0";
  return doCommand(command, callback);
}

function disableNat(params, callback) {
  let command = "nat disable " + params.internalIfname + " " +
                params.externalIfname + " " + "0";
  return doCommand(command, callback);
}

function wifiFirmwareReload(params, callback) {
  let command = "softap fwreload " + params.ifname + " " + params.mode;
  return doCommand(command, callback);
}

function startAccessPointDriver(params, callback) {
  let command = "softap start " + params.ifname;
  return doCommand(command, callback);
}

function stopAccessPointDriver(params, callback) {
  let command = "softap stop " + params.ifname;
  return doCommand(command, callback);
}

function startSoftAP(params, callback) {
  let command = "softap startap";
  return doCommand(command, callback);
}

function stopSoftAP(params, callback) {
  let command = "softap stopap";
  return doCommand(command, callback);
}


function setAccessPoint(params, callback) {
  let command = "softap set " + params.ifname +
                " " + params.wifictrlinterfacename +
                " " + params.ssid +
                " " + params.security +
                " " + params.key +
                " " + "6 0 8";
  return doCommand(command, callback);
}




function setUSBFunction(params) {
  let report = params.report;
  let retry = 0;
  let i = 0;

  libcutils.property_set(USB_FUNCTION_PATH, params.usbfunc);
  
  if (report) {
    setTimeout(checkUSBFunction, USB_FUNCTION_RETRY_INTERVAL, params);
  }

  function checkUSBFunction(params) {
    let result = libcutils.property_get(USB_FUNCTION_STATE);
    if (result == params.usbfunc) {
      params.result = true;
      postMessage(params);
      retry = 0;
      return;
    }
    if (retry < USB_FUNCTION_RETRY_TIMES) {
      retry++;
      setTimeout(checkUSBFunction, USB_FUNCTION_RETRY_INTERVAL, params);
      return;
    }
    params.result = false;
    postMessage(params);
  };

  return true;
}

function dumpParams(params, type) {
  if (!DEBUG) {
    return;
  }

  debug("Dump params:");
  debug("     ifname: " + params.ifname);
  debug("     ip: " + params.ip);
  debug("     link: " + params.link);
  debug("     prefix: " + params.prefix);
  debug("     startIp: " + params.startIp);
  debug("     endIp: " + params.endIp);
  debug("     dnsserver1: " + params.dns1);
  debug("     dnsserver2: " + params.dns2);
  debug("     internalIfname: " + params.internalIfname);
  debug("     externalIfname: " + params.externalIfname);
  if (type == "WIFI") {
    debug("     wifictrlinterfacename: " + params.wifictrlinterfacename);
    debug("     ssid: " + params.ssid);
    debug("     security: " + params.security);
    debug("     key: " + params.key);
  }
}

function chain(params, funcs, onError) {
  let i = -1;
  let f = funcs[i];
  function next(error, result) {
    params.resultCode = result.code;
    params.resultReason = result.reason;
    if (error) {
      if (onError) {
        params.error = error;
        params.state = f.name;
        onError(params);
      }
      return;
    }
    i += 1;
    f = funcs[i];
    if (f) {
      let ret = f(params, next);
      if (!ret && onError) {
        params.error = true;
        params.state = f.name;
        onError(params);
      }
    }
  };
  next(null, {code: NETD_COMMAND_ERROR, reason: ""});
}

let gWifiEnableChain = [wifiFirmwareReload,
                        startAccessPointDriver,
                        setAccessPoint,
                        startSoftAP,
                        setInterfaceUp,
                        tetherInterface,
                        setIpForwardingEnabled,
                        startTethering,
                        setDnsForwarders,
                        enableNat,
                        wifiTetheringSuccess];

let gWifiDisableChain = [stopSoftAP,
                         stopAccessPointDriver,
                         wifiFirmwareReload,
                         disableNat,
                         untetherInterface,
                         setIpForwardingEnabled,
                         stopTethering,
                         wifiTetheringSuccess];




function setWifiTethering(params) {
  let enable = params.enable;
  let interfaceProperties = getIFProperties(params.externalIfname);

  params.dns1 = interfaceProperties.dns1_str;
  params.dns2 = interfaceProperties.dns2_str;
  dumpParams(params, "WIFI");

  if (enable) {
    
    debug("Starting Wifi Tethering on " +
           params.internalIfname + "<->" + params.externalIfname);
    chain(params, gWifiEnableChain, wifiTetheringFail);
  } else {
    
    debug("Stopping Wifi Tethering on " +
           params.internalIfname + "<->" + params.externalIfname);
    chain(params, gWifiDisableChain, wifiTetheringFail);
  }
  return true;
}

let gUSBEnableChain = [setInterfaceUp,
                       enableNat,
                       setIpForwardingEnabled,
                       tetherInterface,
                       startTethering,
                       setDnsForwarders,
                       usbTetheringSuccess];

let gUSBDisableChain = [disableNat,
                        setIpForwardingEnabled,
                        stopTethering,
                        untetherInterface,
                        usbTetheringSuccess];




function setUSBTethering(params) {
  let enable = params.enable;
  let interfaceProperties = getIFProperties(params.externalIfname);

  params.dns1 = interfaceProperties.dns1_str;
  params.dns2 = interfaceProperties.dns2_str;
  dumpParams(params, "USB");

  if (enable) {
    
    debug("Starting USB Tethering on " +
           params.internalIfname + "<->" + params.externalIfname);
    chain(params, gUSBEnableChain, usbTetheringFail);
  } else {
    
    debug("Stopping USB Tethering on " +
           params.internalIfname + "<->" + params.externalIfname);
    chain(params, gUSBDisableChain, usbTetheringFail);
  }
  return true;
}

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("Network Worker: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

