


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


const MAX_DATA_CONTEXTS = 4;

function setEmulatorAPN() {
  
  let apn = [[ { "carrier":"T-Mobile US",
                 "apn":"epc1.tmobile.com",
                 "types":["default"] },
               { "carrier":"T-Mobile US",
                 "apn":"epc2.tmobile.com",
                 "mmsc":"http://mms.msg.eng.t-mobile.com/mms/wapenc",
                 "types":["mms"] },
               { "carrier":"T-Mobile US",
                 "apn":"epc3.tmobile.com",
                 "types":["supl"] },
               { "carrier":"T-Mobile US",
                 "apn":"epc4.tmobile.com",
                 "types":["ims"] },
               { "carrier":"T-Mobile US",
                 "apn":"epc5.tmobile.com",
                 "types":["dun"] }]];

  return setSettings(SETTINGS_KEY_DATA_APN_SETTINGS, apn);
}


function testInitialState() {
  log("= testInitialState =");

  
  return getSettings(SETTINGS_KEY_DATA_ENABLED)
    .then(value => {
      is(value, false, "Data must be off");
    });
}

function testSetupConcurrentDataCalls() {
  log("= testSetupConcurrentDataCalls =");

  let promise = Promise.resolve();
  let types = Object.keys(mobileTypeMapping);
  
  for (let i = 1; i < MAX_DATA_CONTEXTS; i++) {
    let type = types[i];
    promise = promise.then(() => setupDataCallAndWait(type));
  }
  return promise;
}

function testDeactivateConcurrentDataCalls() {
  log("= testDeactivateConcurrentDataCalls =");

  let promise = Promise.resolve();
  let types = Object.keys(mobileTypeMapping);
  
  for (let i = 1; i < MAX_DATA_CONTEXTS; i++) {
    let type = types[i];
    promise = promise.then(() => deactivateDataCallAndWait(type));
  }
  return promise;
}


startTestBase(function() {

  let origApnSettings;
  return testInitialState()
    .then(() => getSettings(SETTINGS_KEY_DATA_APN_SETTINGS))
    .then(value => {
      origApnSettings = value;
    })
    .then(() => setEmulatorAPN())
    .then(() => setDataEnabledAndWait(true))
    .then(() => testSetupConcurrentDataCalls())
    .then(() => testDeactivateConcurrentDataCalls())
    .then(() => setDataEnabledAndWait(false))
    .then(() => {
      if (origApnSettings) {
        return setSettings(SETTINGS_KEY_DATA_APN_SETTINGS, origApnSettings);
      }
    });
});
