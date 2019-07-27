


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


startTestCommon(function() {

  let origApnSettings;
  return getDataApnSettings()
    .then(value => {
      origApnSettings = value;
    })

    
    .then(() => setRadioEnabledAndWait(false))
    .then(() => setRadioEnabledAndWait(true))

    
    .then(() => {
      let apnSettings = [[
        {"carrier":"T-Mobile US",
         "apn":"epc.tmobile.com",
         "mmsc":"http://mms.msg.eng.t-mobile.com/mms/wapenc",
         "types":["default","supl","mms"]}]];
      return setDataApnSettings(apnSettings);
    })
    .then(() => setDataEnabledAndWait(true))
    .then(() => setRadioEnabledAndWait(false))
    .then(() => {
      
      is(mobileConnection.data.connected, false);
    })

    
    .then(() => setDataApnSettings(origApnSettings))
    .then(() => setDataEnabled(false))
    .then(() => setRadioEnabledAndWait(true));

}, ["settings-read", "settings-write", "settings-api-read", "settings-api-write"]);
