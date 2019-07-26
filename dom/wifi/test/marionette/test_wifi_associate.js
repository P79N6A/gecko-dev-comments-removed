


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const SCAN_RETRY_CNT = 5;


















function testAssociate(aNetwork) {
  if (!setPasswordIfNeeded(aNetwork)) {
    throw 'Failed to set password';
  }

  function waitForConnected() {
    return gTestSuite.waitForWifiManagerEventOnce('statuschange')
      .then(function onstatuschange(event) {
        log("event.status: " + event.status);
        log("event.network.ssid: " + (event.network ? event.network.ssid : ''));

        if ("connected" === event.status &&
            event.network.ssid === aNetwork.ssid) {
          return; 
        }

        log('Not expected "connected" statuschange event. Wait again!');
        return waitForConnected();
      });
  }

  let promises = [];

  
  
  promises.push(waitForConnected());

  
  let request = gTestSuite.getWifiManager().associate(aNetwork);
  promises.push(gTestSuite.wrapDomRequestAsPromise(request));

  return Promise.all(promises);
}









function convertToTestAssociateChain(aNetworks) {
  let chain = Promise.resolve();

  aNetworks.forEach(function (aNetwork) {
    chain = chain.then(() => testAssociate(aNetwork));
  });

  return chain;
}













function setPasswordIfNeeded(aNetwork) {
  let i = gTestSuite.getFirstIndexBySsid(aNetwork.ssid, HOSTAPD_CONFIG_LIST);
  if (-1 === i) {
    log('unknown ssid: ' + aNetwork.ssid);
    return false; 
  }

  if (!aNetwork.security.length) {
    return true; 
  }

  let security = aNetwork.security[0];
  if (/PSK$/.test(security)) {
    aNetwork.psk = HOSTAPD_CONFIG_LIST[i].wpa_passphrase;
    aNetwork.keyManagement = 'WPA-PSK';
  } else if (/WEP$/.test(security)) {
    aNetwork.wep = HOSTAPD_CONFIG_LIST[i].wpa_passphrase;
    aNetwork.keyManagement = 'WEP';
  }

  return true;
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
