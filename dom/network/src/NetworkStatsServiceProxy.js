



"use strict";

const DEBUG = false;
function debug(s) { dump("-*- NetworkStatsServiceProxy: " + s + "\n"); }

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["NetworkStatsServiceProxy"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetworkStatsService.jsm");

const NETWORKSTATSSERVICEPROXY_CONTRACTID = "@mozilla.org/networkstatsServiceProxy;1";
const NETWORKSTATSSERVICEPROXY_CID = Components.ID("8fbd115d-f590-474c-96dc-e2b6803ca975");
const nsINetworkStatsServiceProxy = Ci.nsINetworkStatsServiceProxy;

function NetworkStatsServiceProxy() {
  if (DEBUG) {
    debug("Proxy started");
  }
}

NetworkStatsServiceProxy.prototype = {
  



  saveAppStats: function saveAppStats(aAppId, aNetwork, aTimeStamp,
                                      aRxBytes, aTxBytes, aCallback) {
    if (DEBUG) {
      debug("saveAppStats: " + aAppId + " connectionType " + aNetwork.type +
            " " + aTimeStamp + " " + aRxBytes + " " + aTxBytes);
    }

    NetworkStatsService.saveAppStats(aAppId, aNetwork, aTimeStamp,
                                      aRxBytes, aTxBytes, aCallback);
  },

  classID : NETWORKSTATSSERVICEPROXY_CID,
  QueryInterface : XPCOMUtils.generateQI([nsINetworkStatsServiceProxy]),
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NetworkStatsServiceProxy]);
