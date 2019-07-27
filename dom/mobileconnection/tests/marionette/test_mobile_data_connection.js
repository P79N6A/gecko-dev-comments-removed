


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function checkOrWaitForDataState(connected) {
  if (mobileConnection.data.connected == connected) {
    log("data.connected is now " + mobileConnection.data.connected);
    return;
  }

  return waitForManagerEvent("datachange")
    .then(() => checkOrWaitForDataState(connected));
}

function verifyInitialState() {
  log("Verifying initial state.");

  
  return Promise.resolve()
    .then(function() {
      is(mobileConnection.voice.state, "registered", "voice.state");
      is(mobileConnection.data.state, "registered", "data.state");
      is(mobileConnection.voice.roaming, false, "voice.roaming");
      is(mobileConnection.data.roaming, false, "data.roaming");
    })
    .then(getDataEnabled)
    .then(function(aResult) {
      is(aResult, false, "Data must be off.")
    });
}

function testEnableData() {
  log("Turn data on.");

  return setDataEnabledAndWait(true);
}

function testUnregisterDataWhileDataEnabled() {
  log("Set data registration unregistered while data enabled.");

  
  
  return setEmulatorVoiceDataStateAndWait("data", "unregistered")
    .then(() => checkOrWaitForDataState(false));
}

function testRegisterDataWhileDataEnabled() {
  log("Set data registration home while data enabled.");

  
  
  return setEmulatorVoiceDataStateAndWait("data", "home")
    .then(() => checkOrWaitForDataState(true));
}

function testDisableDataRoamingWhileRoaming() {
  log("Disable data roaming while roaming.");

  
  
  return setEmulatorRoamingAndWait(true)
    .then(() => checkOrWaitForDataState(false));
}

function testEnableDataRoamingWhileRoaming() {
  log("Enable data roaming while roaming.");

  
  return setDataRoamingEnabled(true)
    .then(() => checkOrWaitForDataState(true));
}

function testDisableData() {
  log("Turn data off.");

  return setDataEnabledAndWait(false);
}

startTestCommon(function() {

  let origApnSettings;
  return verifyInitialState()
    .then(() => getDataApnSettings())
    .then(value => {
      origApnSettings = value;
    })
    .then(() => {
      let apnSettings = [[{ "carrier": "T-Mobile US",
                            "apn": "epc.tmobile.com",
                            "mmsc": "http://mms.msg.eng.t-mobile.com/mms/wapenc",
                            "types": ["default","supl","mms"] }]];
      return setDataApnSettings(apnSettings);
    })
    .then(() => testEnableData())
    .then(() => testUnregisterDataWhileDataEnabled())
    .then(() => testRegisterDataWhileDataEnabled())
    .then(() => testDisableDataRoamingWhileRoaming())
    .then(() => testEnableDataRoamingWhileRoaming())
    .then(() => testDisableData())
    
    .then(() => {
      if (origApnSettings) {
        return setDataApnSettings(origApnSettings);
      }
    })
    .then(() => setEmulatorRoamingAndWait(false))
    .then(() => setDataRoamingEnabled(false));

}, ["settings-read", "settings-write"]);