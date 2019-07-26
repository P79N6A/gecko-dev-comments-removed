


let Promise = SpecialPowers.Cu.import('resource://gre/modules/Promise.jsm').Promise;

const STOCK_HOSTAPD_NAME = 'goldfish-hostapd';
const HOSTAPD_CONFIG_PATH = '/data/misc/wifi/hostapd/';

const HOSTAPD_COMMON_CONFIG = {
  driver: 'test',
  ctrl_interface: '/data/misc/wifi/hostapd',
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
                       { 'type': 'settings-write', 'allow': 1, 'context': window.document }];

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

  












  function ensureWifiEnabled(aEnabled) {
    if (wifiManager.enabled === aEnabled) {
      log('Already ' + (aEnabled ? 'enabled' : 'disabled'));
      return Promise.resolve();
    }
    return requestWifiEnabled(aEnabled);
  }

  










  function requestWifiEnabled(aEnabled) {
    return Promise.all([
      waitForWifiManagerEventOnce(aEnabled ? 'enabled' : 'disabled'),
      setSettings({ 'wifi.enabled': aEnabled }),
    ]);
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

  















  function setSettings(aSettings, aAllowError) {
    let request = window.navigator.mozSettings.createLock().set(aSettings);
    return wrapDomRequestAsPromise(request)
      .then(function resolve() {
        ok(true, "setSettings(" + JSON.stringify(aSettings) + ")");
      }, function reject() {
        ok(aAllowError, "setSettings(" + JSON.stringify(aSettings) + ")");
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

  













  function cleanUp() {
    waitFor(function() {
      return ensureWifiEnabled(wifiOrigEnabled)
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

  return suite;
})();
