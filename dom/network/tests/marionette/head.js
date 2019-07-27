


let Promise = SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;

const ETHERNET_MANAGER_CONTRACT_ID = "@mozilla.org/ethernetManager;1";

const INTERFACE_UP = "UP";
const INTERFACE_DOWN = "DOWN";

let gTestSuite = (function() {
  let suite = {};

  
  let ethernetManager = SpecialPowers.Cc[ETHERNET_MANAGER_CONTRACT_ID]
                                     .getService(SpecialPowers.Ci.nsIEthernetManager);
  let pendingEmulatorShellCount = 0;

  














  function runEmulatorShellSafe(command) {
    let deferred = Promise.defer();

    ++pendingEmulatorShellCount;
    runEmulatorShell(command, function(aResult) {
      --pendingEmulatorShellCount;

      ok(true, "Emulator shell response: " + JSON.stringify(aResult));
      if (Array.isArray(aResult)) {
        deferred.resolve(aResult);
      } else {
        deferred.reject(aResult);
      }
    });

    return deferred.promise;
  }

  











  function getNetworkConfig(ifname) {
    return runEmulatorShellSafe(['netcfg'])
      .then(result => {
        
        
        
        
        
        

        let config;

        for (let i = 0; i < result.length; i++) {
          let tokens = result[i].split(/\s+/);
          let name = tokens[0];
          let flag = tokens[1];
          let ip = tokens[2].split(/\/+/)[0];
          if (name == ifname) {
            config = { name: name, flag: flag, ip: ip };
            break;
          }
        }

        return config;
      });
  }

  











  function getDhcpIpAddr(ifname) {
    return runEmulatorShellSafe(['getprop', 'dhcp.' + ifname + '.ipaddress'])
      .then(function(ipAddr) {
        return ipAddr[0];
      });
  }

  











  function getDhcpGateway(ifname) {
    return runEmulatorShellSafe(['getprop', 'dhcp.' + ifname + '.gateway'])
      .then(function(gateway) {
        return gateway[0];
      });
  }

  








  function getDefaultRoute() {
    return runEmulatorShellSafe(['ip', 'route'])
      .then(result => {
        
        
        
        

        let routeInfo = [];

        for (let i = 0; i < result.length; i++) {
          if (!result[i].match('default')) {
            continue;
          }

          let tokens = result[i].split(/\s+/);
          let name = tokens[4];
          let gateway = tokens[2];
          routeInfo.push({ name: name, gateway: gateway });
        }

        return routeInfo;
      });
  }

  









  function checkInterfaceIsEnabled(ifname, enabled) {
    return getNetworkConfig(ifname)
      .then(function(config) {
        if (enabled) {
          is(config.flag, INTERFACE_UP, "Interface is enabled as expectation.");
        } else {
          is(config.flag, INTERFACE_DOWN, "Interface is disabled as expectation.");
        }
      });
  }

  









  function checkInterfaceIpAddr(ifname, ip) {
    return getNetworkConfig(ifname)
      .then(function(config) {
        is(config.ip, ip, "IP is right as expectation.");
      });
  }

  










  function checkDefaultRoute(ifname, gateway) {
    return getDefaultRoute()
      .then(function(routeInfo) {
        for (let i = 0; i < routeInfo.length; i++) {
          if (routeInfo[i].name == ifname) {
            is(routeInfo[i].gateway, gateway,
               "Default gateway is right as expectation.");
            return true;
          }
        }

        if (!gateway) {
          ok(true, "Default route is cleared.");
          return true;
        }

        return false;
      });
  }

  






  function checkInterfaceListLength(length) {
    let list = ethernetManager.interfaceList;
    is(length, list.length, "List length is equal as expectation.");
  }

  







  function checkInterfaceExist(ifname) {
    return scanInterfaces()
      .then(list => {
        let index = list.indexOf(ifname);
        if (index < 0) {
          throw "Interface " + ifname + " not found.";
        }

        ok(true, ifname + " exists.")
      });
  }

  






  function scanInterfaces() {
    let deferred = Promise.defer();

    ethernetManager.scan(function onScan(list) {
      deferred.resolve(list);
    });

    return deferred.promise;
  }

  









  function addInterface(ifname) {
    let deferred = Promise.defer();

    ethernetManager.addInterface(ifname, function onAdd(success, message) {
      ok(success, "Add interface " + ifname + " success.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  









  function removeInterface(ifname) {
    let deferred = Promise.defer();

    ethernetManager.removeInterface(ifname, function onRemove(success, message) {
      ok(success, "Remove interface " + ifname + " success.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  









  function enableInterface(ifname) {
    let deferred = Promise.defer();

    ethernetManager.enable(ifname, function onEnable(success, message) {
      ok(success, "Enable interface " + ifname + " success.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  









  function disableInterface(ifname) {
    let deferred = Promise.defer();

    ethernetManager.disable(ifname, function onDisable(success, message) {
      ok(success, "Disable interface " + ifname + " success.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  









  function makeInterfaceConnect(ifname) {
    let deferred = Promise.defer();

    ethernetManager.connect(ifname, function onConnect(success, message) {
      ok(success, "Interface " + ifname + " is connected successfully.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  









  function makeInterfaceDisconnect(ifname) {
    let deferred = Promise.defer();

    ethernetManager.disconnect(ifname, function onDisconnect(success, message) {
      ok(success, "Interface " + ifname + " is disconnected successfully.");
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  















  function updateInterfaceConfig(ifname, config) {
    let deferred = Promise.defer();

    ethernetManager.updateInterfaceConfig(ifname, config,
                                          function onUpdated(success, message) {
      ok(success, "Interface " + ifname + " config is updated successfully " +
                  " with " + JSON.stringify(config));
      is(message, "ok", "Message is as expectation.");

      deferred.resolve(success);
    });

    return deferred.promise;
  }

  







  function waitForTimeout(timeout) {
    let deferred = Promise.defer();

    setTimeout(function() {
      ok(true, "waitForTimeout " + timeout);
      deferred.resolve();
    }, timeout);

    return deferred.promise;
  }

  










  function waitForDefaultRouteSet(ifname, gateway) {
    return gTestSuite.waitForTimeout(500)
      .then(() => gTestSuite.checkDefaultRoute(ifname, gateway))
      .then(success => {
        if (success) {
          ok(true, "Default route is set as expectation " + gateway);
          return;
        }

        ok(true, "Default route is not set yet, check again. " + success);
        return waitForDefaultRouteSet(ifname, gateway);
      });
  }

  
  
  
  suite.scanInterfaces = scanInterfaces;
  suite.addInterface = addInterface;
  suite.removeInterface = removeInterface;
  suite.enableInterface = enableInterface;
  suite.disableInterface = disableInterface;
  suite.makeInterfaceConnect = makeInterfaceConnect;
  suite.makeInterfaceDisconnect = makeInterfaceDisconnect;
  suite.updateInterfaceConfig = updateInterfaceConfig;
  suite.getDhcpIpAddr = getDhcpIpAddr;
  suite.getDhcpGateway = getDhcpGateway;
  suite.checkInterfaceExist = checkInterfaceExist;
  suite.checkInterfaceIsEnabled = checkInterfaceIsEnabled;
  suite.checkInterfaceIpAddr = checkInterfaceIpAddr;
  suite.checkDefaultRoute = checkDefaultRoute;
  suite.checkInterfaceListLength = checkInterfaceListLength;
  suite.waitForTimeout = waitForTimeout;
  suite.waitForDefaultRouteSet = waitForDefaultRouteSet;

  





  function cleanUp() {
    waitFor(finish, function() {
      return pendingEmulatorShellCount === 0;
    });
  }

  











  suite.doTest = function(aTestCaseChain) {
    return Promise.resolve()
      .then(aTestCaseChain)
      .then(function onresolve() {
        cleanUp();
      }, function onreject(aReason) {
        ok(false, 'Promise rejects during test' + (aReason ? '(' + aReason + ')' : ''));
        cleanUp();
      });
  };

  return suite;
})();