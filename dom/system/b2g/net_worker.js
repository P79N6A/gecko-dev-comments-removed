



"use strict";

const DEBUG = true;

importScripts("systemlibs.js");




let ints = ctypes.int.array(8)();








function ipToString(ip) {
  return ((ip >>  0) & 0xFF) + "." +
         ((ip >>  8) & 0xFF) + "." +
         ((ip >> 16) & 0xFF) + "." +
         ((ip >> 24) & 0xFF);
}








function stringToIP(string) {
  let ip = 0;
  let start, end = -1;
  for (let i = 0; i < 4; i++) {
    start = end + 1;
    end = string.indexOf(".", start);
    if (end == -1) {
      end = string.length;
    }
    let num = parseInt(string.slice(start, end), 10);
    if (isNaN(num)) {
      return null;
    }
    ip |= num << (i * 8);
  }
  return ip;
}







function getIFProperties(ifname) {
  let gateway_str = libcutils.property_get("net." + ifname + ".gw");
  return {
    ifname:      ifname,
    gateway:     stringToIP(gateway_str),
    gateway_str: gateway_str,
    dns1_str:    libcutils.property_get("net." + ifname + ".dns1"),
    dns2_str:    libcutils.property_get("net." + ifname + ".dns2"),
  };
}








self.onmessage = function onmessage(event) {
  let message = event.data;
  if (DEBUG) debug("received message: " + JSON.stringify(message));
  let ret = self[message.cmd](message);
  postMessage({id: message.id, ret: ret});
};




function setDefaultRouteAndDNS(options) {
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
  let rv = libnetutils.dhcp_do_request(options.ifname,
                                       ints.addressOfElement(0),
                                       ints.addressOfElement(1),
                                       ints.addressOfElement(2),
                                       ints.addressOfElement(3),
                                       ints.addressOfElement(4),
                                       ints.addressOfElement(5),
                                       ints.addressOfElement(6));
  let options = {
    ifname:         options.ifname,
    ipaddr:         ints[0],
    mask:           ints[2],
    gateway:        ints[1],
    dns1:           ints[3],
    dns2:           ints[4],
    dhcpServer:     ints[5],
    dns1_str:       ipToString(ints[3]),
    dns2_str:       ipToString(ints[4]),
    dhcpLease:      ints[6]
  };

  
  
  
  setDefaultRouteAndDNS(options);
}

if (!this.debug) {
  this.debug = function debug(message) {
    dump("Network Worker: " + message + "\n");
  };
}
