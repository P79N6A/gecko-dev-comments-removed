


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function setEmulatorAPN() {
  let apn = [
    [{"carrier":"T-Mobile US",
      "apn":"epc.tmobile.com",
      "mmsc":"http://mms.msg.eng.t-mobile.com/mms/wapenc",
      "types":["default","supl","mms","ims","dun"]}]
  ];

  return setSettings(SETTINGS_KEY_DATA_APN_SETTINGS, apn);
}


function testInitialState() {
  log("= testInitialState =");

  
  return getSettings(SETTINGS_KEY_DATA_ENABLED)
    .then(value => {
      is(value, false, "Data must be off");
    });
}


function testDefaultDataConnection() {
  log("= testDefaultDataConnection =");

  
  return setDataEnabledAndWait(true)
    
    .then(() => setDataEnabledAndWait(false));
}


function testNonDefaultDataConnection() {
  log("= testNonDefaultDataConnection =");

  function doTestNonDefaultDataConnection(type) {
    log("doTestNonDefaultDataConnection: " + type);

    return setupDataCallAndWait(type)
      .then(() => deactivateDataCallAndWait(type));
  }

  let currentApn;
  return getSettings(SETTINGS_KEY_DATA_APN_SETTINGS)
    .then(value => {
      currentApn = value;
    })
    .then(setEmulatorAPN)
    .then(() => doTestNonDefaultDataConnection("mms"))
    .then(() => doTestNonDefaultDataConnection("supl"))
    .then(() => doTestNonDefaultDataConnection("ims"))
    .then(() => doTestNonDefaultDataConnection("dun"))
    
    .then(() => setSettings(SETTINGS_KEY_DATA_APN_SETTINGS, currentApn));
}


startTestBase(function() {
  return testInitialState()
    .then(() => testDefaultDataConnection())
    .then(() => testNonDefaultDataConnection());
});
