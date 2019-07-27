


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const TESTING_HOSTAPD = [{ ssid: 'ap0' }];

function connectToFirstNetwork() {
  let firstNetwork;
  return gTestSuite.requestWifiScan()
    .then(function (networks) {
      firstNetwork = networks[0];
      return gTestSuite.testAssociate(firstNetwork);
    })
    .then(() => firstNetwork);
}

gTestSuite.doTestTethering(function() {
  let firstNetwork;

  return gTestSuite.ensureWifiEnabled(true)
    
    .then(() => gTestSuite.startHostapds(TESTING_HOSTAPD))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', TESTING_HOSTAPD.length))

    
    .then(function () {
      return Promise.all([
        
        
        gTestSuite.waitForRilDataConnected(false),

        
        connectToFirstNetwork()
          .then(aFirstNetwork => firstNetwork = aFirstNetwork)
      ]);
    })
    .then(function() {
      return Promise.all([
        
        
        gTestSuite.waitForWifiManagerEventOnce('disabled'),
        gTestSuite.waitForRilDataConnected(true),

        
        gTestSuite.requestTetheringEnabled(true)
      ]);
    })
    .then(function() {
      return Promise.all([
        
        
        
        gTestSuite.waitForWifiManagerEventOnce('enabled'),

        
        
        
        
        

        
        gTestSuite.requestTetheringEnabled(false)
      ]);
    })
    
    
    .then(() => gTestSuite.requestWifiEnabled(false))

    
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0));
});