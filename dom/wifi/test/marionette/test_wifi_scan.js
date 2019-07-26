


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const SCAN_RETRY_CNT = 5;










function testScanNoAp() {
  return gTestSuite.testWifiScanWithRetry(SCAN_RETRY_CNT, []);
}










function testScanWithAps() {
  return gTestSuite.startHostapds(HOSTAPD_CONFIG_LIST)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', HOSTAPD_CONFIG_LIST.length))
    .then(() => gTestSuite.testWifiScanWithRetry(SCAN_RETRY_CNT, HOSTAPD_CONFIG_LIST))
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0));
}

gTestSuite.doTestWithoutStockAp(function() {
  return gTestSuite.ensureWifiEnabled(true)
    .then(testScanNoAp)
    .then(testScanWithAps);
});
