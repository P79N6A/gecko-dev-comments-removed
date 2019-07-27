



"use strict";







function isNetworkReady() {
  
  if ("nsINetworkInterfaceListService" in SpecialPowers.Ci) {
    var listService = SpecialPowers.Cc["@mozilla.org/network/interface-list-service;1"]
                        .getService(SpecialPowers.Ci.nsINetworkInterfaceListService);
    var itfList = listService.getDataInterfaceList(
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_MMS_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_SUPL_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_IMS_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_DUN_INTERFACES);
    var num = itfList.getNumberOfInterface();
    for (var i = 0; i < num; i++) {
      var ips = {};
      var prefixLengths = {};
      var length = itfList.getInterface(i).getAddresses(ips, prefixLengths);

      for (var j = 0; j < length; j++) {
        var ip = ips.value[j];
        
        if (ip.indexOf(":") < 0) {
          safeInfo("Network interface is ready with address: " + ip);
          return true;
        }
      }
    }
    
    safeInfo("Network interface is not ready, required additional network setup");
    return false;
  }
  safeInfo("Network setup is not required");
  return true;
}






function getNetworkUtils() {
  var url = SimpleTest.getTestFileURL("NetworkPreparationChromeScript.js");
  var script = SpecialPowers.loadChromeScript(url);

  var utils = {
    




    prepareNetwork: function() {
      return new Promise(resolve => {
        script.addMessageListener('network-ready', () =>  {
          info("Network interface is ready");
          resolve();
        });
        info("Setting up network interface");
        script.sendAsyncMessage("prepare-network", true);
      });
    },
    




    tearDownNetwork: function() {
      if (!isNetworkReady()) {
        info("No network to tear down");
        return Promise.resolve();
      }
      return new Promise(resolve => {
        script.addMessageListener('network-disabled', message => {
          info("Network interface torn down");
          script.destroy();
          resolve();
        });
        info("Tearing down network interface");
        script.sendAsyncMessage("network-cleanup", true);
      });
    }
  };

  return utils;
}





function startNetworkAndTest() {
  if (isNetworkReady()) {
    return Promise.resolve();
  }
  var utils = getNetworkUtils();
  
  return utils.prepareNetwork();
}




function networkTestFinished() {
  var p;
  if ("nsINetworkInterfaceListService" in SpecialPowers.Ci) {
    var utils = getNetworkUtils();
    p = utils.tearDownNetwork();
  } else {
    p = Promise.resolve();
  }
  return p.then(() => SimpleTest.finish());
}
