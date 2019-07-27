




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

add_test(function check_linktype() {
  
  let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
  if (network.isLinkUp) {
    ok(network.linkType != Ci.nsINetworkLinkService.LINK_TYPE_UNKNOWN, "LinkType is not UNKNOWN");
  } else {
    ok(network.linkType == Ci.nsINetworkLinkService.LINK_TYPE_UNKNOWN, "LinkType is UNKNOWN");
  }

  run_next_test();
});

run_next_test();
