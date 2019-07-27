



MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

gTestSuite.startTest(function() {
  let origApnSettings;
  return gTestSuite.getDataApnSettings()
    .then(value => {
      origApnSettings = value;
    })
    .then(() => {
      
      let apnSettings = [[ { "carrier": "T-Mobile US",
                             "apn": "epc1.tmobile.com",
                             "mmsc": "http://mms.msg.eng.t-mobile.com/mms/wapenc",
                             "types": ["default","supl","mms"] },
                           { "carrier": "T-Mobile US",
                             "apn": "epc2.tmobile.com",
                             "types": ["dun"] } ]];
      return gTestSuite.setDataApnSettings(apnSettings);
    })
    .then(() => gTestSuite.setTetheringDunRequired())
    .then(() => gTestSuite.startTetheringTest(function() {
      return gTestSuite.ensureWifiEnabled(false)
        .then(() => gTestSuite.setWifiTetheringEnabled(true, true))
        .then(() => gTestSuite.setWifiTetheringEnabled(false, true));
    }))
    
    .then(() => {
      if (origApnSettings) {
        return gTestSuite.setDataApnSettings(origApnSettings);
      }
    });
});