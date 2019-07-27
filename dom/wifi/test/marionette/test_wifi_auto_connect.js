


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const TESTING_HOSTAPD = [{ ssid: 'ap0' }];

gTestSuite.doTestWithoutStockAp(function() {
  let firstNetwork;
  return gTestSuite.ensureWifiEnabled(true)
    
    .then(() => gTestSuite.startHostapds(TESTING_HOSTAPD))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', TESTING_HOSTAPD.length))

    
    .then(gTestSuite.requestWifiScan)
    .then(function(networks) {
      firstNetwork = networks[0];
      return gTestSuite.testAssociate(firstNetwork);
    })

    
    

    
    .then(() => gTestSuite.requestWifiEnabled(false))
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0))

    
    .then(() => gTestSuite.requestWifiEnabled(true))

    
    .then(() => gTestSuite.startHostapds(TESTING_HOSTAPD))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', TESTING_HOSTAPD.length))

    
    .then(() => gTestSuite.waitForConnected(firstNetwork))

    
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0))
});
