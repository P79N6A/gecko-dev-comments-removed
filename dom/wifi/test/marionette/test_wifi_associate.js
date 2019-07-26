


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const SCAN_RETRY_CNT = 5;









function convertToTestAssociateChain(aNetworks) {
  let chain = Promise.resolve();

  aNetworks.forEach(function (aNetwork) {
    chain = chain.then(() => gTestSuite.testAssociate(aNetwork));
  });

  return chain;
}

gTestSuite.doTestWithoutStockAp(function() {
  return gTestSuite.ensureWifiEnabled(true)
    .then(() => gTestSuite.startHostapds(HOSTAPD_CONFIG_LIST))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', HOSTAPD_CONFIG_LIST.length))
    .then(() => gTestSuite.testWifiScanWithRetry(SCAN_RETRY_CNT, HOSTAPD_CONFIG_LIST))
    .then(networks => convertToTestAssociateChain(networks))
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0));
});
