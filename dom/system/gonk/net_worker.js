














"use strict";

const DEBUG = false;

const PERSIST_SYS_USB_CONFIG_PROPERTY = "persist.sys.usb.config";
const SYS_USB_CONFIG_PROPERTY         = "sys.usb.config";
const SYS_USB_STATE_PROPERTY          = "sys.usb.state";

const USB_FUNCTION_RNDIS  = "rndis";
const USB_FUNCTION_ADB    = "adb";

const kNetdInterfaceChangedTopic = "netd-interface-change";
const kNetdBandwidthControlTopic = "netd-bandwidth-control";


const DUMMY_COMMAND = "tether status";


const USB_FUNCTION_RETRY_TIMES = 20;

const USB_FUNCTION_RETRY_INTERVAL = 100;


const NETD_COMMAND_PROCEEDING   = 100;

const NETD_COMMAND_OKAY         = 200;


const NETD_COMMAND_FAIL         = 400;

const NETD_COMMAND_ERROR        = 500;

const NETD_COMMAND_UNSOLICITED  = 600;


const NETD_COMMAND_INTERFACE_CHANGE     = 600;
const NETD_COMMAND_BANDWIDTH_CONTROLLER = 601;

const INTERFACE_DELIMIT = "\0";

importScripts("systemlibs.js");

const SDK_VERSION = libcutils.property_get("ro.build.version.sdk", "0");

function netdResponseType(code) {
  return Math.floor(code/100)*100;
}

function isBroadcastMessage(code) {
  let type = netdResponseType(code);
  return (type == NETD_COMMAND_UNSOLICITED);
}

function isError(code) {
  let type = netdResponseType(code);
  return (type != NETD_COMMAND_PROCEEDING && type != NETD_COMMAND_OKAY);
}

function isComplete(code) {
  let type = netdResponseType(code);
  return (type != NETD_COMMAND_PROCEEDING);
}

function isProceeding(code) {
  let type = netdResponseType(code);
  return (type === NETD_COMMAND_PROCEEDING);
}

function sendBroadcastMessage(code, reason) {
  let topic = null;
  switch (code) {
    case NETD_COMMAND_INTERFACE_CHANGE:
      topic = "netd-interface-change";
      break;
    case NETD_COMMAND_BANDWIDTH_CONTROLLER:
      topic = "netd-bandwidth-control";
      break;
  }

  if (topic) {
    postMessage({id: 'broadcast', topic: topic, reason: reason});
  }
}

let gWifiFailChain = [stopSoftAP,
                      setIpForwardingEnabled,
                      stopTethering];

function wifiTetheringFail(params) {
  
  postMessage(params);

  
  

  
  params.enable = false;
  chain(params, gWifiFailChain, null);
}

function wifiTetheringSuccess(params) {
  
  postMessage(params);
  return true;
}

let gUSBFailChain = [stopSoftAP,
                     setIpForwardingEnabled,
                     stopTethering];

function usbTetheringFail(params) {
  
  postMessage(params);
  
  
  
  params.enable = false;
  chain(params, gUSBFailChain, null);

  
  enableUsbRndis({enable: false, report: false});
}

function usbTetheringSuccess(params) {
  
  postMessage(params);
  return true;
}

function networkInterfaceStatsFail(params) {
  
  postMessage(params);
  return true;
}

function networkInterfaceStatsSuccess(params) {
  
  params.txBytes = parseFloat(params.resultReason);

  postMessage(params);
  return true;
}

function updateUpStreamSuccess(params) {
  
  postMessage(params);
  return true;
}

function updateUpStreamFail(params) {
  
  postMessage(params);
  return true;
}

function wifiOperationModeFail(params) {
  
  postMessage(params);
  return true;
}

function wifiOperationModeSuccess(params) {
  
  postMessage(params);
  return true;
}







function getIFProperties(ifname) {
  return {
    ifname:      ifname,
    gateway_str: libcutils.property_get("net." + ifname + ".gw"),
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




function setDNS(options) {
  let ifprops = getIFProperties(options.ifname);
  let dns1_str = options.dns1_str || ifprops.dns1_str;
  let dns2_str = options.dns2_str || ifprops.dns2_str;
  libcutils.property_set("net.dns1", dns1_str);
  libcutils.property_set("net.dns2", dns2_str);
  
  let dnschange = libcutils.property_get("net.dnschange", "0");
  libcutils.property_set("net.dnschange", (parseInt(dnschange, 10) + 1).toString());
}




function setDefaultRouteAndDNS(options) {
  if (options.oldIfname) {
    libnetutils.ifc_remove_default_route(options.oldIfname);
  }

  let ifprops = getIFProperties(options.ifname);
  let gateway_str = options.gateway_str || ifprops.gateway_str;
  let dns1_str = options.dns1_str || ifprops.dns1_str;
  let dns2_str = options.dns2_str || ifprops.dns2_str;
  let gateway = netHelpers.stringToIP(gateway_str);

  libnetutils.ifc_set_default_route(options.ifname, gateway);
  libcutils.property_set("net.dns1", dns1_str);
  libcutils.property_set("net.dns2", dns2_str);

  
  let dnschange = libcutils.property_get("net.dnschange", "0");
  libcutils.property_set("net.dnschange", (parseInt(dnschange, 10) + 1).toString());
}




function removeDefaultRoute(options) {
  libnetutils.ifc_remove_default_route(options.ifname);
}




function addHostRoute(options) {
  for (let i = 0; i < options.hostnames.length; i++) {
    libnetutils.ifc_add_route(options.ifname, options.hostnames[i], 32, options.gateway);
  }
}




function removeHostRoute(options) {
  for (let i = 0; i < options.hostnames.length; i++) {
    libnetutils.ifc_remove_route(options.ifname, options.hostnames[i], 32, options.gateway);
  }
}




function removeHostRoutes(options) {
  libnetutils.ifc_remove_host_routes(options.ifname);
}

function removeNetworkRoute(options) {
  let ipvalue = netHelpers.stringToIP(options.ip);
  let netmaskvalue = netHelpers.stringToIP(options.netmask);
  let subnet = netmaskvalue & ipvalue;
  let dst = netHelpers.ipToString(subnet);
  let prefixLength = netHelpers.getMaskLength(netmaskvalue);
  let gateway = "0.0.0.0";

  libnetutils.ifc_remove_default_route(options.ifname);
  libnetutils.ifc_remove_route(options.ifname, dst, prefixLength, gateway);
}

let gCommandQueue = [];
let gCurrentCommand = null;
let gCurrentCallback = null;
let gPending = false;
let gReason = [];








function split(start, data, delimiter) {
  
  if (start < 0 || data.length <= 0) {
    return null;
  }

  let result = "";
  let i = start;
  while (i < data.length) {
    let octet = data[i];
    i += 1;
    if (octet === delimiter) {
      return {token: result, index: i};
    }
    result += String.fromCharCode(octet);
  }
  return null;
}




function onNetdMessage(data) {
  let result = split(0, data, 32);
  if (!result) {
    nextNetdCommand();
    return;
  }
  let code = parseInt(result.token);

  
  
  
  if (!isBroadcastMessage(code) && SDK_VERSION >= 16) {
    result = split(result.index, data, 32);
  }

  let i = result.index;
  let reason = "";
  for (; i < data.length; i++) {
    let octet = data[i];
    reason += String.fromCharCode(octet);
  }

  if (isBroadcastMessage(code)) {
    debug("Receiving broadcast message from netd.");
    debug("          ==> Code: " + code + "  Reason: " + reason);
    sendBroadcastMessage(code, reason);
    nextNetdCommand();
    return;
  }

  
  debug("Receiving '" + gCurrentCommand + "' command response from netd.");
  debug("          ==> Code: " + code + "  Reason: " + reason);

  gReason.push(reason);

  
  
  if (isProceeding(code)) {
    return;
  }

  if (isComplete(code)) {
    gPending = false;
  }

  if (gCurrentCallback) {
    gCurrentCallback(isError(code),
                     {code: code, reason: gReason.join(INTERFACE_DELIMIT)});
    gReason = [];
  }

  
  if (isComplete(code)) {
    nextNetdCommand();
  }
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

  
  let command = (SDK_VERSION >= 16) ? "0 " + gCurrentCommand : gCurrentCommand;
  return postNetdCommand(command);
}

function setInterfaceUp(params, callback) {
  let command = "interface setcfg " + params.ifname + " " + params.ip + " " +
                params.prefix + " ";
  if (SDK_VERSION >= 16) {
    command += params.link;
  } else {
    command += "[" + params.link + "]";
  }
  return doCommand(command, callback);
}

function setInterfaceDown(params, callback) {
  let command = "interface setcfg " + params.ifname + " " + params.ip + " " +
                params.prefix + " ";
  if (SDK_VERSION >= 16) {
    command += params.link;
  } else {
    command += "[" + params.link + "]";
  }
  return doCommand(command, callback);
}

function setIpForwardingEnabled(params, callback) {
  let command;

  if (params.enable) {
    command = "ipfwd enable";
  } else {
    
    
    if ("interfaceList" in params && params.interfaceList.length > 1) {
      command = DUMMY_COMMAND;
    } else {
      command = "ipfwd disable";
    }
  }
  return doCommand(command, callback);
}

function startTethering(params, callback) {
  let command;
  
  
  if (params.resultReason.indexOf("started") !== -1) {
    command = DUMMY_COMMAND;
  } else {
    command = "tether start " + params.wifiStartIp + " " + params.wifiEndIp +
              " " + params.usbStartIp + " " + params.usbEndIp;
  }
  return doCommand(command, callback);
}

function tetheringStatus(params, callback) {
  let command = "tether status";
  return doCommand(command, callback);
}

function stopTethering(params, callback) {
  let command;

  
  
  if ("interfaceList" in params && params.interfaceList.length > 1) {
    command = DUMMY_COMMAND;
  } else {
    command = "tether stop";
  }
  return doCommand(command, callback);
}

function tetherInterface(params, callback) {
  let command = "tether interface add " + params.ifname;
  return doCommand(command, callback);
}

function preTetherInterfaceList(params, callback) {
  let command = (SDK_VERSION >= 16) ? "tether interface list"
                                    : "tether interface list 0";
  return doCommand(command, callback);
}

function postTetherInterfaceList(params, callback) {
  params.interfaceList = params.resultReason.split(INTERFACE_DELIMIT);

  
  let command = DUMMY_COMMAND;
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
  let command;

  
  
  if ("interfaceList" in params && params.interfaceList.length > 1) {
    command = DUMMY_COMMAND;
  } else {
    command = "nat disable " + params.internalIfname + " " +
              params.externalIfname + " " + "0";
  }
  return doCommand(command, callback);
}

function wifiFirmwareReload(params, callback) {
  let command = "softap fwreload " + params.ifname + " " + params.mode;
  return doCommand(command, callback);
}

function startAccessPointDriver(params, callback) {
  
  if (SDK_VERSION >= 16) {
    callback(false, {code: "", reason: ""});
    return true;
  }
  let command = "softap start " + params.ifname;
  return doCommand(command, callback);
}

function stopAccessPointDriver(params, callback) {
  
  if (SDK_VERSION >= 16) {
    callback(false, {code: "", reason: ""});
    return true;
  }
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

function getRxBytes(params, callback) {
  let command = "interface readrxcounter " + params.ifname;
  return doCommand(command, callback);
}

function getTxBytes(params, callback) {
  params.rxBytes = parseFloat(params.resultReason);

  let command = "interface readtxcounter " + params.ifname;
  return doCommand(command, callback);
}

function escapeQuote(str) {
  str = str.replace(/\\/g, "\\\\");
  return str.replace(/"/g, "\\\"");
}



















function setAccessPoint(params, callback) {
  let command;
  if (SDK_VERSION >= 16) {
    command = "softap set " + params.ifname +
              " \"" + escapeQuote(params.ssid) + "\"" +
              " " + params.security +
              " \"" + escapeQuote(params.key) + "\"";
  } else {
    command = "softap set " + params.ifname +
              " " + params.wifictrlinterfacename +
              " \"" + escapeQuote(params.ssid) + "\"" +
              " " + params.security +
              " \"" + escapeQuote(params.key) + "\"" +
              " " + "6 0 8";
  }
  return doCommand(command, callback);
}

function cleanUpStream(params, callback) {
  let command = "nat disable " + params.previous.internalIfname + " " +
                params.previous.externalIfname + " " + "0";
  return doCommand(command, callback);
}

function createUpStream(params, callback) {
  let command = "nat enable " + params.current.internalIfname + " " +
                params.current.externalIfname + " " + "0";
  return doCommand(command, callback);
}




function enableUsbRndis(params) {
  let report = params.report;
  let retry = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  let currentConfig = libcutils.property_get(SYS_USB_CONFIG_PROPERTY);
  let configFuncs = currentConfig.split(",");
  let persistConfig = libcutils.property_get(PERSIST_SYS_USB_CONFIG_PROPERTY);
  let persistFuncs = persistConfig.split(",");

  if (params.enable) {
    configFuncs = [USB_FUNCTION_RNDIS];
    if (persistFuncs.indexOf(USB_FUNCTION_ADB) >= 0) {
      configFuncs.push(USB_FUNCTION_ADB);
    }
  } else {
    
    
    
    configFuncs = persistFuncs;
  }
  let newConfig = configFuncs.join(",");
  if (newConfig != currentConfig) {
    libcutils.property_set(SYS_USB_CONFIG_PROPERTY, newConfig);
  }

  
  if (report) {
    setTimeout(checkUsbRndisState, USB_FUNCTION_RETRY_INTERVAL, params);
  }

  function checkUsbRndisState(params) {
    let currentState = libcutils.property_get(SYS_USB_STATE_PROPERTY);
    let stateFuncs = currentState.split(",");
    let rndisPresent = (stateFuncs.indexOf(USB_FUNCTION_RNDIS) >= 0);
    if (params.enable == rndisPresent) {
      params.result = true;
      postMessage(params);
      retry = 0;
      return;
    }
    if (retry < USB_FUNCTION_RETRY_TIMES) {
      retry++;
      setTimeout(checkUsbRndisState, USB_FUNCTION_RETRY_INTERVAL, params);
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
  debug("     wifiStartIp: " + params.wifiStartIp);
  debug("     wifiEndIp: " + params.wifiEndIp);
  debug("     usbStartIp: " + params.usbStartIp);
  debug("     usbEndIp: " + params.usbEndIp);
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
                        tetheringStatus,
                        startTethering,
                        setDnsForwarders,
                        enableNat,
                        wifiTetheringSuccess];

let gWifiDisableChain = [stopSoftAP,
                         stopAccessPointDriver,
                         wifiFirmwareReload,
                         untetherInterface,
                         preTetherInterfaceList,
                         postTetherInterfaceList,
                         disableNat,
                         setIpForwardingEnabled,
                         stopTethering,
                         wifiTetheringSuccess];




function setWifiTethering(params) {
  let enable = params.enable;
  let interfaceProperties = getIFProperties(params.externalIfname);

  if (interfaceProperties.dns1_str) {
    params.dns1 = interfaceProperties.dns1_str;
  }
  if (interfaceProperties.dns2_str) {
    params.dns2 = interfaceProperties.dns2_str;
  }
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

let gUpdateUpStreamChain = [cleanUpStream,
                            createUpStream,
                            updateUpStreamSuccess];



function updateUpStream(params) {
  chain(params, gUpdateUpStreamChain, updateUpStreamFail);
}

let gUSBEnableChain = [setInterfaceUp,
                       enableNat,
                       setIpForwardingEnabled,
                       tetherInterface,
                       tetheringStatus,
                       startTethering,
                       setDnsForwarders,
                       usbTetheringSuccess];

let gUSBDisableChain = [untetherInterface,
                        preTetherInterfaceList,
                        postTetherInterfaceList,
                        disableNat,
                        setIpForwardingEnabled,
                        stopTethering,
                        usbTetheringSuccess];




function setUSBTethering(params) {
  let enable = params.enable;
  let interfaceProperties = getIFProperties(params.externalIfname);

  if (interfaceProperties.dns1_str) {
    params.dns1 = interfaceProperties.dns1_str;
  }
  if (interfaceProperties.dns2_str) {
    params.dns2 = interfaceProperties.dns2_str;
  }

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

let gNetworkInterfaceStatsChain = [getRxBytes,
                                   getTxBytes,
                                   networkInterfaceStatsSuccess];




function getNetworkInterfaceStats(params) {
  debug("getNetworkInterfaceStats: " + params.ifname);

  params.rxBytes = -1;
  params.txBytes = -1;
  params.date = new Date();

  chain(params, gNetworkInterfaceStatsChain, networkInterfaceStatsFail);
  return true;
}

let gWifiOperationModeChain = [wifiFirmwareReload,
                               wifiOperationModeSuccess];




function setWifiOperationMode(params) {
  debug("setWifiOperationMode: " + params.ifname + " " + params.mode);
  chain(params, gWifiOperationModeChain, wifiOperationModeFail);
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

