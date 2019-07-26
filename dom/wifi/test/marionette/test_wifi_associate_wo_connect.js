


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

















function associateButDontConnect(aNetwork) {
  log('Associating with ' + aNetwork.ssid);
  aNetwork.dontConnect = true;

  let promises = [];
  promises.push(gTestSuite.waitForTimeout(10 * 1000)
                  .then(() => { throw 'timeout'; }));

  promises.push(gTestSuite.testAssociate(aNetwork));

  return Promise.all(promises)
    .then(() => { throw 'unexpected state'; },
          function(aReason) {
      is(typeof aReason, 'string', 'typeof aReason');
      is(aReason, 'timeout', aReason);
    });
}

gTestSuite.doTest(function() {
  let firstNetwork;
  return gTestSuite.ensureWifiEnabled(true)
    .then(gTestSuite.requestWifiScan)
    .then(function(aNetworks) {
      firstNetwork = aNetworks[0];
      return associateButDontConnect(firstNetwork);
    })
    .then(gTestSuite.getKnownNetworks)
    .then(function(aKnownNetworks) {
      is(1, aKnownNetworks.length, 'There should be only one known network!');
      is(aKnownNetworks[0].ssid, firstNetwork.ssid,
         'The only one known network should be ' + firstNetwork.ssid)
    });
});
