


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";





















function doTest(aApnSettings, aIsIPv6) {
  return setDataApnSettings([])
    .then(() => setDataApnSettings(aApnSettings))
    .then(() => setDataEnabledAndWait(true))
    .then(function() {
      let nm = getNetworkManager();
      let active = nm.active;
      ok(active, "Active network interface");

      log("  Interface: " + active.name);
      log("  Address: " + active.ip);
      if (aIsIPv6) {
        ok(active.ip.indexOf(":") > 0, "IPv6 address");
      } else {
        ok(active.ip.indexOf(":") < 0, "IPv4 address");
      }
    })
    .then(() => setDataEnabledAndWait(false));
}

function doTestHome(aApnSettings, aProtocol) {
  log("Testing \"" + aProtocol + "\"@HOME... ");

  
  
  
  
  aApnSettings[0][0].protocol = aProtocol;
  delete aApnSettings[0][0].roaming_protocol;

  return doTest(aApnSettings, aProtocol === "IPV6");
}

function doTestRoaming(aApnSettings, aRoaminProtocol) {
  log("Testing \"" + aRoaminProtocol + "\"@ROMAING... ");

  
  
  
  
  delete aApnSettings[0][0].protocol;
  aApnSettings[0][0].roaming_protocol = aRoaminProtocol;

  return doTest(aApnSettings, aRoaminProtocol === "IPV6");
}

startTestCommon(function() {
  let origApnSettings;

  return setDataRoamingEnabled(true)
    .then(getDataApnSettings)
    .then(function(aResult) {
      
      origApnSettings = JSON.parse(JSON.stringify(aResult));
      return aResult;
    }, function() {
      
      return [[{ "carrier": "T-Mobile US",
                 "apn": "epc.tmobile.com",
                 "mmsc": "http://mms.msg.eng.t-mobile.com/mms/wapenc",
                 "types": ["default", "supl", "mms"] }]];
    })

    .then(function(aApnSettings) {
      return Promise.resolve()

        .then(() => doTestHome(aApnSettings, "NoSuchProtocol"))
        .then(() => doTestHome(aApnSettings, "IP"))
        .then(() => doTestHome(aApnSettings, "IPV6"))

        .then(() => setEmulatorRoamingAndWait(true))

        .then(() => doTestRoaming(aApnSettings, "NoSuchProtocol"))
        .then(() => doTestRoaming(aApnSettings, "IP"))
        .then(() => doTestRoaming(aApnSettings, "IPV6"))

        .then(() => setEmulatorRoamingAndWait(false));
    })

    .then(() => setDataRoamingEnabled(false))
    .then(function() {
      if (origApnSettings) {
        return setDataApnSettings(origApnSettings);
      }
    });
}, ["settings-read", "settings-write"]);
