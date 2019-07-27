



"use strict";

const DEBUG = false;
function debug(s) { dump("-*- NetworkStatsServiceProxy: " + s + "\n"); }

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["NetworkStatsServiceProxy"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetworkStatsService.jsm");

const NETWORKSTATSSERVICEPROXY_CONTRACTID = "@mozilla.org/networkstatsServiceProxy;1";
const NETWORKSTATSSERVICEPROXY_CID = Components.ID("98fd8f69-784e-4626-aa59-56d6436a3c24");
const nsINetworkStatsServiceProxy = Ci.nsINetworkStatsServiceProxy;

function NetworkStatsServiceProxy() {
  if (DEBUG) {
    debug("Proxy started");
  }
}

NetworkStatsServiceProxy.prototype = {
  



  saveAppStats: function saveAppStats(aAppId, aIsInBrowser, aNetwork, aTimeStamp,
                                      aRxBytes, aTxBytes, aIsAccumulative,
                                      aCallback) {
    if (!aNetwork) {
      if (DEBUG) {
        debug("|aNetwork| is not specified. Failed to save stats. Returning.");
      }
      return;
    }

    if (DEBUG) {
      debug("saveAppStats: " + aAppId + " " + aIsInBrowser + " " +
            aNetwork.type + " " + aTimeStamp + " " +
            aRxBytes + " " + aTxBytes + " " + aIsAccumulative);
    }

    if (aCallback) {
      aCallback = aCallback.notify;
    }

    NetworkStatsService.saveStats(aAppId, aIsInBrowser, "", aNetwork,
                                  aTimeStamp, aRxBytes, aTxBytes,
                                  aIsAccumulative, aCallback);
  },

  



  saveServiceStats: function saveServiceStats(aServiceType, aNetwork,
                                              aTimeStamp, aRxBytes, aTxBytes,
                                              aIsAccumulative, aCallback) {
    if (!aNetwork) {
      if (DEBUG) {
        debug("|aNetwork| is not specified. Failed to save stats. Returning.");
      }
      return;
    }

    if (DEBUG) {
      debug("saveServiceStats: " + aServiceType + " " + aNetwork.type + " " +
            aTimeStamp + " " + aRxBytes + " " + aTxBytes + " " +
            aIsAccumulative);
    }

    if (aCallback) {
      aCallback = aCallback.notify;
    }

    NetworkStatsService.saveStats(0, false, aServiceType ,aNetwork, aTimeStamp,
                                  aRxBytes, aTxBytes, aIsAccumulative,
                                  aCallback);
  },

  classID : NETWORKSTATSSERVICEPROXY_CID,
  QueryInterface : XPCOMUtils.generateQI([nsINetworkStatsServiceProxy]),
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([NetworkStatsServiceProxy]);
