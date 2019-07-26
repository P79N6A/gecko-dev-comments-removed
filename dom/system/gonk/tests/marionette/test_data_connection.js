


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_CONTEXT = "chrome";

Cu.import("resource://gre/modules/Promise.jsm");

const DATA_KEY = "ril.data.enabled";
const APN_KEY  = "ril.data.apnSettings";

let ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
ok(ril, "ril.constructor is " + ril.constructor);

let radioInterface = ril.getRadioInterface(0);
ok(radioInterface, "radioInterface.constructor is " + radioInterface.constrctor);

function setSetting(key, value) {
  log("setSetting: '" + key + "'' -> " + JSON.stringify(value));

  let deferred = Promise.defer();
  let obj = {};
  obj[key] = value;

  let setRequest = window.navigator.mozSettings.createLock().set(obj);
  setRequest.addEventListener("success", function() {
    log("set '" + key + "' to " + JSON.stringify(value) + " success");
    deferred.resolve();
  });
  setRequest.addEventListener("error", function() {
    ok(false, "cannot set '" + key + "' to " + JSON.stringify(value));
    deferred.reject();
  });

  return deferred.promise;
}

function getSetting(key) {
  log("getSetting: '" + key + "'");

  let deferred = Promise.defer();

  let getRequest = window.navigator.mozSettings.createLock().get(key);
  getRequest.addEventListener("success", function() {
    let result = getRequest.result[key];
	  log("setting '" + key + "': " + JSON.stringify(result));
	  deferred.resolve(result);
  });
  getRequest.addEventListener("error", function() {
    ok(false, "cannot get '" + key + "'");
    deferred.reject();
  });

  return deferred.promise;
}

function setEmulatorAPN() {
  let apn = [
    [{"carrier":"T-Mobile US",
      "apn":"epc.tmobile.com",
      "mmsc":"http://mms.msg.eng.t-mobile.com/mms/wapenc",
      "types":["default","supl","mms","ims","dun"]}]
  ];

  return setSetting(APN_KEY, apn);
}

function waitNetworkConnected(networkType) {
  log("wait network " + networkType + " connected");

  let interfaceStateChangeTopic = "network-connection-state-changed";
  let obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  let deferred = Promise.defer();

  function observer(subject, topic, data) {
    let network = subject.QueryInterface(Ci.nsINetworkInterface);
    log("Network " + network.type + " state changes to " + network.state);
    if (network.type == networkType &&
        network.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED) {
      obs.removeObserver(observer, interfaceStateChangeTopic);
      deferred.resolve();
    }
  }

  obs.addObserver(observer, interfaceStateChangeTopic, false);

  return deferred.promise;
}

function waitNetworkDisconnected(networkType) {
  log("wait network " + networkType + " disconnected");

  let interfaceStateChangeTopic = "network-connection-state-changed";
  let obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  let deferred = Promise.defer();

  function observer(subject, topic, data) {
    let network = subject.QueryInterface(Ci.nsINetworkInterface);
    log("Network " + network.type + " state changes to " + network.state);
    
    
    
    if (network.state == Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED ||
        network.state == Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN) {
      obs.removeObserver(observer, interfaceStateChangeTopic);
      deferred.resolve();
    }
  }

  obs.addObserver(observer, interfaceStateChangeTopic, false);

  return deferred.promise;
}


function testInitialState() {
  log("= testInitialState =");

  
  return Promise.resolve()
    .then(() => getSetting(DATA_KEY))
    .then(value => {
      is(value, false, "Data must be off");
    })
    .then(null, () => {
      ok(false, "promise rejected during test");
    })
    .then(runNextTest);
}


function testDefaultDataConnection() {
  log("= testDefaultDataConnection =");

  return Promise.resolve()
    
    .then(() => setSetting(DATA_KEY, true))
    .then(() => waitNetworkConnected(Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE))
    
    .then(() => setSetting(DATA_KEY, false))
    .then(() => waitNetworkDisconnected(Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE))
    .then(null, () => {
      ok(false, "promise rejected during test");
    })
    .then(runNextTest);
}


function testNonDefaultDataConnection() {
  log("= testNonDefaultDataConnection =");

  function doTestNonDefaultDataConnection(type) {
    log("doTestNonDefaultDataConnection: " + type);

    let typeMapping = {
      "mms": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS,
      "supl": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,
      "ims": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS,
      "dun": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN
    };
    let networkType = typeMapping[type];

    return Promise.resolve()
      .then(() => radioInterface.setupDataCallByType(type))
      .then(() => waitNetworkConnected(networkType))
      .then(() => radioInterface.deactivateDataCallByType(type))
      .then(() => waitNetworkDisconnected(networkType));
  }

  let currentApn;
  return Promise.resolve()
    .then(() => getSetting(APN_KEY))
    .then(value => {
      currentApn = value;
    })
    .then(setEmulatorAPN)
    .then(() => doTestNonDefaultDataConnection("mms"))
    .then(() => doTestNonDefaultDataConnection("supl"))
    .then(() => doTestNonDefaultDataConnection("ims"))
    .then(() => doTestNonDefaultDataConnection("dun"))
    
    .then(() => setSetting(APN_KEY, currentApn))
    .then(null, () => {
      ok(false, "promise rejected during test");
    })
    .then(runNextTest);
}

let tests = [
  testInitialState,
  testDefaultDataConnection,
  testNonDefaultDataConnection
];

function runNextTest() {
  let test = tests.shift();
  if (!test) {
    finish();
    return;
  }

  test();
}


runNextTest();
