


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

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
        gTestSuite.waitForRilDataConnected(false),
        gTestSuite.waitForConnected(firstNetwork),

        
        gTestSuite.requestTetheringEnabled(false)
      ]);
    })
    
    
    .then(() => gTestSuite.requestWifiEnabled(false));
});