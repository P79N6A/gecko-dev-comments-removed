



const TYPE_WIFI = "wifi";
const TYPE_BLUETOOTH = "bt";
const TYPE_USB = "usb";




const TETHERING_SETTING_IP = "192.168.1.1";
const TETHERING_SETTNG_PREFIX = "24";
const TETHERING_SETTING_START_IP = "192.168.1.10";
const TETHERING_SETTING_END_IP = "192.168.1.30";
const TETHERING_SETTING_DNS1 = "8.8.8.8";
const TETHERING_SETTING_DNS2 = "8.8.4.4";




const TETHERING_SETTING_SSID = "FirefoxHotSpot";
const TETHERING_SETTING_SECURITY = "open";
const TETHERING_SETTING_KEY = "1234567890";

const SETTINGS_RIL_DATA_ENABLED = 'ril.data.enabled';

let Promise =
  SpecialPowers.Cu.import("resource://gre/modules/Promise.jsm").Promise;

let gTestSuite = (function() {
  let suite = {};

  let tetheringManager;
  let pendingEmulatorShellCount = 0;

  















  function isOrThrow(value1, value2, message) {
    is(value1, value2, message);
    if (value1 !== value2) {
      throw message;
    }
  }

  
















  function runEmulatorShellSafe(aCommand) {
    let deferred = Promise.defer();

    ++pendingEmulatorShellCount;
    runEmulatorShell(aCommand, function(aResult) {
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

  











  function waitForTimeout(aTimeoutMs) {
    let deferred = Promise.defer();

    setTimeout(function() {
      deferred.resolve();
    }, aTimeoutMs);

    return deferred.promise;
  }

  














  function getSettings(aKey) {
    let request = navigator.mozSettings.createLock().get(aKey);

    return wrapDomRequestAsPromise(request)
      .then(function resolve(aEvent) {
        ok(true, "getSettings(" + aKey + ") - success");
        return aEvent.target.result[aKey];
      }, function reject(aEvent) {
        ok(false, "getSettings(" + aKey + ") - error");
        throw aEvent.target.error;
      });
  }

  











  function setSettings(aSettings) {
    let request = navigator.mozSettings.createLock().set(aSettings);

    return wrapDomRequestAsPromise(request)
      .then(function resolve() {
        ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
      }, function reject(aEvent) {
        ok(false, "setSettings(" + JSON.stringify(aSettings) + ")");
        throw aEvent.target.error;
      });
  }

  

















  function setSettings1(aKey, aValue, aAllowError) {
    let settings = {};
    settings[aKey] = aValue;
    return setSettings(settings, aAllowError);
  }

  










  function wrapDomRequestAsPromise(aRequest) {
    let deffered = Promise.defer();

    ok(aRequest instanceof DOMRequest,
      "aRequest is instanceof" + aRequest.constructor);

    aRequest.onsuccess = function(aEvent) {
      deffered.resolve(aEvent);
    };
    aRequest.onerror = function(aEvent) {
      deffered.reject(aEvent);
    };

    return deffered.promise;
  }

  











  function waitForMobileConnectionEventOnce(aEventName, aServiceId) {
    aServiceId = aServiceId || 0;

    let deferred = Promise.defer();
    let mobileconnection = navigator.mozMobileConnections[aServiceId];

    mobileconnection.addEventListener(aEventName, function onevent(aEvent) {
      mobileconnection.removeEventListener(aEventName, onevent);

      ok(true, "Mobile connection event '" + aEventName + "' got.");
      deferred.resolve(aEvent);
    });

    return deferred.promise;
  }

  

















  function waitForRilDataConnected(aConnected, aServiceId) {
    aServiceId = aServiceId || 0;

    return waitForMobileConnectionEventOnce('datachange', aServiceId)
      .then(function () {
        let mobileconnection = navigator.mozMobileConnections[aServiceId];
        if (mobileconnection.data.connected !== aConnected) {
          return waitForRilDataConnected(aConnected, aServiceId);
        }
      });
  }

  

















  function verifyTetheringRouting(aEnabled) {
    let netcfgResult = {};
    let ipRouteResult = {};

    
    
    function exeAndParseNetcfg() {
      return runEmulatorShellSafe(['netcfg'])
        .then(function (aLines) {
          
          
          
          
          
          
          
          
          
          
          
          aLines.forEach(function (aLine) {
            let tokens = aLine.split(/\s+/);
            if (tokens.length < 5) {
              return;
            }
            let ifname = tokens[0];
            let ip = (tokens[2].split('/'))[0];
            netcfgResult[ifname] = { ip: ip };
          });
        });
    }

    
    
    function exeAndParseIpRoute() {
      return runEmulatorShellSafe(['ip', 'route'])
        .then(function (aLines) {
          
          
          
          
          
          
          
          
          
          

          
          aLines.forEach(function (aLine) {
            let tokens = aLine.trim().split(/\s+/);
            let srcIndex = tokens.indexOf('src');
            if (srcIndex < 0 || srcIndex + 1 >= tokens.length) {
              return;
            }
            let ifname = tokens[2];
            let src = tokens[srcIndex + 1];
            ipRouteResult[ifname] = { src: src, default: false };
          });

          
          aLines.forEach(function (aLine) {
            let tokens = aLine.split(/\s+/);
            if (tokens.length < 2) {
              return;
            }
            if ('default' === tokens[0]) {
              let ifnameIndex = tokens.indexOf('dev');
              if (ifnameIndex < 0 || ifnameIndex + 1 >= tokens.length) {
                return;
              }
              let ifname = tokens[ifnameIndex + 1];
              if (ipRouteResult[ifname]) {
                ipRouteResult[ifname].default = true;
              }
              return;
            }
          });

        });

    }

    
    
    
    function verifyIptables() {
      return runEmulatorShellSafe(['iptables', '-t', 'nat', '-L', 'POSTROUTING'])
        .then(function(aLines) {
          
          
          
          
          
          
          
          
          let found = (function find_MASQUERADE() {
            
            for (let i = 2; i < aLines.length; i++) {
              if (-1 !== aLines[i].indexOf('MASQUERADE')) {
                return true;
              }
            }
            return false;
          })();

          if ((aEnabled && !found) || (!aEnabled && found)) {
            throw 'MASQUERADE' + (found ? '' : ' not') + ' found while tethering is ' +
                  (aEnabled ? 'enabled' : 'disabled');
          }
        });
    }

    function verifyDefaultRouteAndIp(aExpectedWifiTetheringIp) {
      log(JSON.stringify(ipRouteResult));
      log(JSON.stringify(netcfgResult));

      if (aEnabled) {
        isOrThrow(ipRouteResult['rmnet0'].src, netcfgResult['rmnet0'].ip, 'rmnet0.ip');
        isOrThrow(ipRouteResult['rmnet0'].default, true, 'rmnet0.default');

        isOrThrow(ipRouteResult['wlan0'].src, netcfgResult['wlan0'].ip, 'wlan0.ip');
        isOrThrow(ipRouteResult['wlan0'].src, aExpectedWifiTetheringIp, 'expected ip');
        isOrThrow(ipRouteResult['wlan0'].default, false, 'wlan0.default');
      }
    }

    return verifyIptables()
      .then(exeAndParseNetcfg)
      .then(exeAndParseIpRoute)
      .then(() => verifyDefaultRouteAndIp(TETHERING_SETTING_IP));
  }

  














  function setWifiTetheringEnabled(aEnabled) {
    let RETRY_INTERVAL_MS = 1000;
    let retryCnt = 20;

    let config = {
      "ip"        : TETHERING_SETTING_IP,
      "prefix"    : TETHERING_SETTNG_PREFIX,
      "startIp"   : TETHERING_SETTING_START_IP,
      "endIp"     : TETHERING_SETTING_END_IP,
      "dns1"      : TETHERING_SETTING_DNS1,
      "dns2"      : TETHERING_SETTING_DNS2,
      "wifiConfig": {
        "ssid"      : TETHERING_SETTING_SSID,
        "security"  : TETHERING_SETTING_SECURITY
      }
    };

    return tetheringManager.setTetheringEnabled(aEnabled, TYPE_WIFI, config)
      .then(function waitForRoutingVerified() {
        return verifyTetheringRouting(aEnabled)
          .then(null, function onreject(aReason) {

            log('verifyTetheringRouting rejected due to ' + aReason +
                ' (' + retryCnt + ')');

            if (!retryCnt--) {
              throw aReason;
            }

            return waitForTimeout(RETRY_INTERVAL_MS).then(waitForRoutingVerified);
          });
      });
  }

  










  function ensureWifiEnabled(aEnabled) {
    let wifiManager = window.navigator.mozWifiManager;
    if (wifiManager.enabled === aEnabled) {
      return Promise.resolve();
    }
    let request = wifiManager.setWifiEnabled(aEnabled);
    return wrapDomRequestAsPromise(request)
  }

  










  function ensureTetheringManager() {
    let deferred = Promise.defer();

    tetheringManager = window.navigator.mozTetheringManager;

    if (tetheringManager instanceof MozTetheringManager) {
      deferred.resolve();
    } else {
      log("navigator.mozTetheringManager is unavailable");
      deferred.reject();
    }

    return deferred.promise;
  }

  









  function acquirePermission() {
    let deferred = Promise.defer();

    let permissions = [{ 'type': 'wifi-manage', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-write', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-read', 'allow': 1, 'context': window.document },
                       { 'type': 'mobileconnection', 'allow': 1, 'context': window.document }];

    SpecialPowers.pushPermissions(permissions, function() {
      deferred.resolve();
    });

    return deferred.promise;
  }

  














  suite.startTest = function(aTestCaseChain) {
    function setUp() {
      return ensureTetheringManager()
        .then(acquirePermission);
    }

    function tearDown() {
      waitFor(finish, function() {
        return pendingEmulatorShellCount === 0;
      });
    }

    return setUp()
      .then(aTestCaseChain)
      .then(function onresolve() {
        tearDown();
      }, function onreject(aReason) {
        ok(false, 'Promise rejects during test' + (aReason ? '(' + aReason + ')' : ''));
        tearDown();
      });
  };

  
  
  
  suite.ensureWifiEnabled = ensureWifiEnabled;
  suite.setWifiTetheringEnabled = setWifiTetheringEnabled;

  

















  suite.startTetheringTest = function(aTestCaseChain) {
    let oriDataEnabled;
    function verifyInitialState() {
      return getSettings(SETTINGS_RIL_DATA_ENABLED)
        .then(enabled => initTetheringTestEnvironment(enabled));
    }

    function initTetheringTestEnvironment(aEnabled) {
      oriDataEnabled = aEnabled;
      if (aEnabled) {
        return Promise.resolve();
      } else {
        return Promise.all([waitForRilDataConnected(true),
                            setSettings1(SETTINGS_RIL_DATA_ENABLED, true)]);
      }
    }

    function restoreToInitialState() {
      return setSettings1(SETTINGS_RIL_DATA_ENABLED, oriDataEnabled);
    }

    return suite.startTest(function() {
      return verifyInitialState()
        .then(aTestCaseChain)
        .then(restoreToInitialState, function onreject(aReason) {
          return restoreToInitialState()
            .then(() => { throw aReason; }); 
        });
    });
  };

  return suite;
})();
