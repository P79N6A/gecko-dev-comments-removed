


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";





















function doTest(aApnSettings, aHaveV4Address, aHaveV6Address) {
  return setDataApnSettings([])
    .then(() => setDataApnSettings(aApnSettings))
    .then(() => setDataEnabledAndWait(true))
    .then(function() {
      let nm = getNetworkManager();
      let active = nm.active;
      ok(active, "Active network interface");
      log("  Interface: " + active.name);

      let ips = {}, prefixLengths = {};
      let num = active.getAddresses(ips, prefixLengths);
      log("  Num addresses: " + num);
      log("  Addresses: " + JSON.stringify(ips.value));
      log("  PrefixLengths: " + JSON.stringify(prefixLengths.value));

      if (aHaveV4Address) {
        ok(ips.value.reduce(function(aFound, aAddress) {
          return aFound || aAddress.indexOf(":") < 0;
        }), "IPv4 address");
      }
      if (aHaveV6Address) {
        ok(ips.value.reduce(function(aFound, aAddress) {
          return aFound || aAddress.indexOf(":") > 0;
        }), "IPv6 address");
      }
    })
    .then(() => setDataEnabledAndWait(false));
}

function doTestHome(aApnSettings, aProtocol) {
  log("Testing \"" + aProtocol + "\"@HOME... ");

  
  
  
  
  aApnSettings[0][0].protocol = aProtocol;
  delete aApnSettings[0][0].roaming_protocol;

  return doTest(aApnSettings,
                aProtocol === "IP" || aProtocol === "IPV4V6",
                aProtocol === "IPV4V6" || aProtocol === "IPV6");
}

function doTestRoaming(aApnSettings, aRoaminProtocol) {
  log("Testing \"" + aRoaminProtocol + "\"@ROMAING... ");

  
  
  
  
  delete aApnSettings[0][0].protocol;
  aApnSettings[0][0].roaming_protocol = aRoaminProtocol;

  return doTest(aApnSettings,
                aRoaminProtocol === "IP" || aRoaminProtocol === "IPV4V6",
                aRoaminProtocol === "IPV4V6" || aRoaminProtocol === "IPV6");
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
        .then(() => doTestHome(aApnSettings, "IPV4V6"))
        .then(() => doTestHome(aApnSettings, "IPV6"))

        .then(() => setEmulatorRoamingAndWait(true))

        .then(() => doTestRoaming(aApnSettings, "NoSuchProtocol"))
        .then(() => doTestRoaming(aApnSettings, "IP"))
        .then(() => doTestRoaming(aApnSettings, "IPV4V6"))
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
