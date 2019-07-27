


let Promise = SpecialPowers.Cu.import('resource://gre/modules/Promise.jsm').Promise;

const STOCK_HOSTAPD_NAME = 'goldfish-hostapd';
const HOSTAPD_CONFIG_PATH = '/data/misc/wifi/remote-hostapd/';

const SETTINGS_RIL_DATA_ENABLED = 'ril.data.enabled';
const SETTINGS_TETHERING_WIFI_ENABLED = 'tethering.wifi.enabled';
const SETTINGS_TETHERING_WIFI_IP = 'tethering.wifi.ip';
const SETTINGS_TETHERING_WIFI_SECURITY = 'tethering.wifi.security.type';

const HOSTAPD_COMMON_CONFIG = {
  driver: 'test',
  ctrl_interface: '/data/misc/wifi/remote-hostapd',
  test_socket: 'DIR:/data/misc/wifi/sockets',
  hw_mode: 'b',
  channel: '2',
};

const HOSTAPD_CONFIG_LIST = [
  { ssid: 'ap0' },

  { ssid: 'ap1',
    wpa: 1,
    wpa_pairwise: 'TKIP CCMP',
    wpa_passphrase: '12345678'
  },

  { ssid: 'ap2',
    wpa: 2,
    rsn_pairwise: 'CCMP',
    wpa_passphrase: '12345678',
  },
];

let gTestSuite = (function() {
  let suite = {};

  
  let wifiManager;
  let wifiOrigEnabled;
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

  











  function waitForWifiManagerEventOnce(aEventName) {
    let deferred = Promise.defer();

    wifiManager.addEventListener(aEventName, function onevent(aEvent) {
      wifiManager.removeEventListener(aEventName, onevent);

      ok(true, "WifiManager event '" + aEventName + "' got.");
      deferred.resolve(aEvent);
    });

    return deferred.promise;
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

  












  function getProcessDetail(aProcessName) {
    return runEmulatorShellSafe(['ps'])
      .then(processes => {
        
        
        
        
        
        
        
        let detail = [];

        processes.shift(); 
        for (let i = 0; i < processes.length; i++) {
          let tokens = processes[i].split(/\s+/);
          let pname = tokens[tokens.length - 1];
          let pid = tokens[1];
          if (-1 !== pname.indexOf(aProcessName)) {
            detail.push({ pname: pname, pid: pid });
          }
        }

        return detail;
      });
  }

  









  function addRequiredPermissions() {
    let deferred = Promise.defer();

    let permissions = [{ 'type': 'wifi-manage', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-write', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-read', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-api-write', 'allow': 1, 'context': window.document },
                       { 'type': 'settings-api-read', 'allow': 1, 'context': window.document },
                       { 'type': 'mobileconnection', 'allow': 1, 'context': window.document }];

    SpecialPowers.pushPermissions(permissions, function() {
      deferred.resolve();
    });

    return deferred.promise;
  }

  










  function wrapDomRequestAsPromise(aRequest) {
    let deferred = Promise.defer();

    ok(aRequest instanceof DOMRequest,
       "aRequest is instanceof " + aRequest.constructor);

    aRequest.addEventListener("success", function(aEvent) {
      deferred.resolve(aEvent);
    });
    aRequest.addEventListener("error", function(aEvent) {
      deferred.reject(aEvent);
    });

    return deferred.promise;
  }

  












  function ensureWifiEnabled(aEnabled, useAPI) {
    if (wifiManager.enabled === aEnabled) {
      log('Already ' + (aEnabled ? 'enabled' : 'disabled'));
      return Promise.resolve();
    }
    return requestWifiEnabled(aEnabled, useAPI);
  }

  


















  function requestWifiEnabled(aEnabled, useAPI) {
    return Promise.all([
      waitForWifiManagerEventOnce(aEnabled ? 'enabled' : 'disabled'),
      useAPI ?
        wrapDomRequestAsPromise(wifiManager.setWifiEnabled(aEnabled)) :
        setSettings({ 'wifi.enabled': aEnabled }),
    ]);
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

  














  function requestTetheringEnabled(aEnabled) {
    let RETRY_INTERVAL_MS = 1000;
    let retryCnt = 20;

    return setSettings1(SETTINGS_TETHERING_WIFI_ENABLED, aEnabled)
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

  













  function forgetNetwork(aNetwork) {
    let request = wifiManager.forget(aNetwork);
    return wrapDomRequestAsPromise(request)
      .then(event => event.target.result);
  }

  










  function forgetAllKnownNetworks() {

    function createForgetNetworkChain(aNetworks) {
      let chain = Promise.resolve();

      aNetworks.forEach(function (aNetwork) {
        chain = chain.then(() => forgetNetwork(aNetwork));
      });

      return chain;
    }

    return getKnownNetworks()
      .then(networks => createForgetNetworkChain(networks));
  }

  










  function getKnownNetworks() {
    let request = wifiManager.getKnownNetworks();
    return wrapDomRequestAsPromise(request)
      .then(event => event.target.result);
  }

  










  function requestWifiScan() {
    let request = wifiManager.getNetworks();
    return wrapDomRequestAsPromise(request)
      .then(event => event.target.result);
  }

  

















  function testWifiScanWithRetry(aRetryCnt, aExpectedNetworks) {

    
    
    function isScanResultExpected(aScanResult) {
      if (aScanResult.length !== aExpectedNetworks.length) {
        return false;
      }

      for (let i = 0; i < aScanResult.length; i++) {
        if (-1 === getFirstIndexBySsid(aScanResult[i].ssid, aExpectedNetworks)) {
          return false;
        }
      }
      return true;
    }

    return requestWifiScan()
      .then(function (networks) {
        if (isScanResultExpected(networks, aExpectedNetworks)) {
          return networks;
        }
        if (aRetryCnt > 0) {
          return testWifiScanWithRetry(aRetryCnt - 1, aExpectedNetworks);
        }
        throw 'Unexpected scan result!';
      });
  }

  
















  function testAssociate(aNetwork) {
    setPasswordIfNeeded(aNetwork);

    let promises = [];

    
    
    promises.push(waitForConnected(aNetwork));

    
    let request = wifiManager.associate(aNetwork);
    promises.push(wrapDomRequestAsPromise(request));

    return Promise.all(promises);
  }

  function waitForConnected(aExpectedNetwork) {
    return waitForWifiManagerEventOnce('statuschange')
      .then(function onstatuschange(event) {
        log("event.status: " + event.status);
        log("event.network.ssid: " + (event.network ? event.network.ssid : ''));

        if ("connected" === event.status &&
            event.network.ssid === aExpectedNetwork.ssid) {
          return; 
        }

        log('Not expected "connected" statuschange event. Wait again!');
        return waitForConnected(aExpectedNetwork);
      });
  }

  








  function setPasswordIfNeeded(aNetwork) {
    let i = getFirstIndexBySsid(aNetwork.ssid, HOSTAPD_CONFIG_LIST);
    if (-1 === i) {
      log('unknown ssid: ' + aNetwork.ssid);
      return; 
    }

    if (!aNetwork.security.length) {
      return; 
    }

    let security = aNetwork.security[0];
    if (/PSK$/.test(security)) {
      aNetwork.psk = HOSTAPD_CONFIG_LIST[i].wpa_passphrase;
      aNetwork.keyManagement = 'WPA-PSK';
    } else if (/WEP$/.test(security)) {
      aNetwork.wep = HOSTAPD_CONFIG_LIST[i].wpa_passphrase;
      aNetwork.keyManagement = 'WEP';
    }
  }

  















  function setSettings(aSettings, aAllowError) {
    let request = window.navigator.mozSettings.createLock().set(aSettings);
    return wrapDomRequestAsPromise(request)
      .then(function resolve() {
        ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
      }, function reject() {
        ok(aAllowError, "setSettings(" + JSON.stringify(aSettings) + ")");
      });
  }

  

















  function setSettings1(aKey, aValue, aAllowError) {
    let settings = {};
    settings[aKey] = aValue;
    return setSettings(settings, aAllowError);
  }

  

















  function getSettings(aKey, aAllowError) {
    let request =
      navigator.mozSettings.createLock().get(aKey);
    return wrapDomRequestAsPromise(request)
      .then(function resolve(aEvent) {
        ok(true, "getSettings(" + aKey + ") - success");
        return aEvent.target.result[aKey];
      }, function reject(aEvent) {
        ok(aAllowError, "getSettings(" + aKey + ") - error");
      });
  }


  



















  function startHostapds(aConfigList) {

    function createConfigFromCommon(aIndex) {
      
      let config = JSON.parse(JSON.stringify(HOSTAPD_COMMON_CONFIG));

      
      for (let key in aConfigList[aIndex]) {
        config[key] = aConfigList[aIndex][key];
      }

      
      
      config.interface = 'AP-' + aIndex;

      return config;
    }

    function startOneHostapd(aIndex) {
      let configFileName = HOSTAPD_CONFIG_PATH + 'ap' + aIndex + '.conf';
      return writeHostapdConfFile(configFileName, createConfigFromCommon(aIndex))
        .then(() => runEmulatorShellSafe(['hostapd', '-B', configFileName]))
        .then(function (reply) {
          
          if (reply[0] === 'bind(PF_UNIX): Address already in use') {
            return startOneHostapd(aIndex);
          }
        });
    }

    return Promise.all(aConfigList.map(function(aConfig, aIndex) {
      return startOneHostapd(aIndex);
    }));
  }

  








  function killAllHostapd() {
    return getProcessDetail('hostapd')
      .then(function (runningHostapds) {
        let promises = runningHostapds.map(runningHostapd => {
          return runEmulatorShellSafe(['kill', '-9', runningHostapd.pid]);
        });
        return Promise.all(promises);
      });
  }

  


















  function writeHostapdConfFile(aFilePath, aConfig) {
    let content = '';
    for (let key in aConfig) {
      if (aConfig.hasOwnProperty(key)) {
        content += (key + '=' + aConfig[key] + '\n');
      }
    }
    return writeFile(aFilePath, content);
  }

  
















  function writeFile(aFilePath, aContent) {
    if (-1 === aContent.indexOf(' ')) {
      aContent = '"' + aContent + '"';
    }
    return runEmulatorShellSafe(['echo', aContent, '>', aFilePath]);
  }

  















  function isInitServiceRunning(aServiceName) {
    return runEmulatorShellSafe(['getprop', 'init.svc.' + aServiceName])
      .then(function (result) {
        if ('running' !== result[0] && 'stopped' !== result[0]) {
          throw 'Init service running state should be "running" or "stopped".';
        }
        return 'running' === result[0];
      });
  }

  











  function waitForTimeout(aTimeoutMs) {
    let deferred = Promise.defer();

    setTimeout(function() {
      deferred.resolve();
    }, aTimeoutMs);

    return deferred.promise;
  }

  

















  function startStopInitService(aServiceName, aStart) {
    let retryCnt = 5;

    return runEmulatorShellSafe([aStart ? 'start' : 'stop', aServiceName])
      .then(() => isInitServiceRunning(aServiceName))
      .then(function onIsServiceRunningResolved(aIsRunning) {
        if (aStart === aIsRunning) {
          return;
        }

        if (retryCnt-- > 0) {
          log('Failed to ' + (aStart ? 'start ' : 'stop ') + aServiceName +
              '. Retry: ' + retryCnt);

          return waitForTimeout(500)
            .then(() => isInitServiceRunning(aServiceName))
            .then(onIsServiceRunningResolved);
        }

        throw 'Failed to ' + (aStart ? 'start' : 'stop') + ' ' + aServiceName;
      });
  }

  













  function startStockHostapd() {
    return startStopInitService(STOCK_HOSTAPD_NAME, true)
      .then(null, function onreject() {
        log('Failed to restart goldfish-hostapd at the first time. Try again!');
        return startStopInitService((STOCK_HOSTAPD_NAME), true);
      });
  }

  










  function stopStockHostapd() {
    return startStopInitService(STOCK_HOSTAPD_NAME, false);
  }

  












  function getFirstIndexBySsid(aSsid, aArray) {
    for (let i = 0; i < aArray.length; i++) {
      if (aArray[i].ssid === aSsid) {
        return i;
      }
    }
    return -1;
  }

  













  function verifyNumOfProcesses(aProcessName, aExpectedNum) {
    return getProcessDetail(aProcessName)
      .then(function (detail) {
        if (detail.length === aExpectedNum) {
          return;
        }
        throw 'Unexpected number of running processes:' + aProcessName +
              ', expected: ' + aExpectedNum + ', actual: ' + detail.length;
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
      .then(() => getSettings(SETTINGS_TETHERING_WIFI_IP))
      .then(ip => verifyDefaultRouteAndIp(ip));
  }

  













  function cleanUp() {
    waitFor(function() {
      return ensureWifiEnabled(true)
        .then(forgetAllKnownNetworks)
        .then(() => ensureWifiEnabled(wifiOrigEnabled))
        .then(finish);
    }, function() {
      return pendingEmulatorShellCount === 0;
    });
  }

  













  function initTestEnvironment() {
    return addRequiredPermissions()
      .then(function() {
        wifiManager = window.navigator.mozWifiManager;
        if (!wifiManager) {
          throw 'window.navigator.mozWifiManager is NULL';
        }
        wifiOrigEnabled = wifiManager.enabled;
      });
  }

  
  
  
  suite.getWifiManager = (() => wifiManager);
  suite.ensureWifiEnabled = ensureWifiEnabled;
  suite.requestWifiEnabled = requestWifiEnabled;
  suite.startHostapds = startHostapds;
  suite.getProcessDetail = getProcessDetail;
  suite.killAllHostapd = killAllHostapd;
  suite.wrapDomRequestAsPromise = wrapDomRequestAsPromise;
  suite.waitForWifiManagerEventOnce = waitForWifiManagerEventOnce;
  suite.verifyNumOfProcesses = verifyNumOfProcesses;
  suite.testWifiScanWithRetry = testWifiScanWithRetry;
  suite.getFirstIndexBySsid = getFirstIndexBySsid;
  suite.testAssociate = testAssociate;
  suite.getKnownNetworks = getKnownNetworks;
  suite.requestWifiScan = requestWifiScan;
  suite.waitForConnected = waitForConnected;
  suite.forgetNetwork = forgetNetwork;
  suite.waitForTimeout = waitForTimeout;
  suite.waitForRilDataConnected = waitForRilDataConnected;
  suite.requestTetheringEnabled = requestTetheringEnabled;

  














  suite.doTest = function(aTestCaseChain) {
    return initTestEnvironment()
      .then(aTestCaseChain)
      .then(function onresolve() {
        cleanUp();
      }, function onreject(aReason) {
        ok(false, 'Promise rejects during test' + (aReason ? '(' + aReason + ')' : ''));
        cleanUp();
      });
  };

  













  suite.doTestWithoutStockAp = function(aTestCaseChain) {
    return suite.doTest(function() {
      return stopStockHostapd()
        .then(aTestCaseChain)
        .then(startStockHostapd);
    });
  };

  

















  suite.doTestTethering = function(aTestCaseChain) {

    function verifyInitialState() {
      return getSettings(SETTINGS_RIL_DATA_ENABLED)
        .then(enabled => isOrThrow(enabled, false, SETTINGS_RIL_DATA_ENABLED))
        .then(() => getSettings(SETTINGS_TETHERING_WIFI_ENABLED))
        .then(enabled => isOrThrow(enabled, false, SETTINGS_TETHERING_WIFI_ENABLED));
    }

    function initTetheringTestEnvironment() {
      
      return Promise.all([waitForRilDataConnected(true),
                          setSettings1(SETTINGS_RIL_DATA_ENABLED, true)])
        .then(setSettings1(SETTINGS_TETHERING_WIFI_SECURITY, 'open'));
    }

    function restoreToInitialState() {
      return setSettings1(SETTINGS_RIL_DATA_ENABLED, false)
        .then(() => getSettings(SETTINGS_TETHERING_WIFI_ENABLED))
        .then(enabled => is(enabled, false, 'Tethering should be turned off.'));
    }

    return suite.doTest(function() {
      return verifyInitialState()
        .then(initTetheringTestEnvironment)
        .then(aTestCaseChain)
        .then(restoreToInitialState, function onreject(aReason) {
          return restoreToInitialState()
            .then(() => { throw aReason; }); 
        });
    });
  };

  return suite;
})();
