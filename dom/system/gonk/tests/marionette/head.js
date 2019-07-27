


MARIONETTE_CONTEXT = "chrome";

const SETTINGS_KEY_DATA_ENABLED = "ril.data.enabled";
const SETTINGS_KEY_DATA_APN_SETTINGS  = "ril.data.apnSettings";

const TOPIC_CONNECTION_STATE_CHANGED = "network-connection-state-changed";
const TOPIC_NETWORK_ACTIVE_CHANGED = "network-active-changed";

const NETWORK_STATE_UNKNOWN = Ci.nsINetworkInterface.NETWORK_STATE_UNKNOWN;
const NETWORK_STATE_CONNECTING = Ci.nsINetworkInterface.NETWORK_STATE_CONNECTING;
const NETWORK_STATE_CONNECTED = Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;
const NETWORK_STATE_DISCONNECTING = Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTING;
const NETWORK_STATE_DISCONNECTED = Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED;

const NETWORK_TYPE_MOBILE = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;
const NETWORK_TYPE_MOBILE_MMS = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS;
const NETWORK_TYPE_MOBILE_SUPL = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL;
const NETWORK_TYPE_MOBILE_IMS = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS;
const NETWORK_TYPE_MOBILE_DUN = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN;

const networkTypes = [
  NETWORK_TYPE_MOBILE,
  NETWORK_TYPE_MOBILE_MMS,
  NETWORK_TYPE_MOBILE_SUPL,
  NETWORK_TYPE_MOBILE_IMS,
  NETWORK_TYPE_MOBILE_DUN
];

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;

let ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
ok(ril, "ril.constructor is " + ril.constructor);

let radioInterface = ril.getRadioInterface(0);
ok(radioInterface, "radioInterface.constructor is " + radioInterface.constrctor);


















function getSettings(aKey, aAllowError) {
  let request = window.navigator.mozSettings.createLock().get(aKey);
  return request.then(function resolve(aValue) {
      log("getSettings(" + aKey + ") - success");
      return aValue[aKey];
    }, function reject(aError) {
      ok(aAllowError, "getSettings(" + aKey + ") - error");
    });
}



















function setSettings(aKey, aValue, aAllowError) {
  let settings = {};
  settings[aKey] = aValue;
  let lock = window.navigator.mozSettings.createLock();
  let request = lock.set(settings);
  let deferred = Promise.defer();
  lock.onsettingstransactionsuccess = function () {
      log("setSettings(" + JSON.stringify(settings) + ") - success");
    deferred.resolve();
  };
  lock.onsettingstransactionfailure = function () {
      ok(aAllowError, "setSettings(" + JSON.stringify(settings) + ") - error");
    
    
    deferred.resolve();
  };
  return deferred.promise;
}













function waitForObserverEvent(aTopic) {
  let obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  let deferred = Promise.defer();

  obs.addObserver(function observer(subject, topic, data) {
    if (topic === aTopic) {
      obs.removeObserver(observer, aTopic);
      deferred.resolve(subject);
    }
  }, aTopic, false);

  return deferred.promise;
}












function setDataEnabledAndWait(aEnabled) {
  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, NETWORK_TYPE_MOBILE,
         "subject.type should be " + NETWORK_TYPE_MOBILE);
      is(aSubject.state,
         aEnabled ? Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED
                  : Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED,
         "subject.state should be " + aEnabled ? "CONNECTED" : "DISCONNECTED");
    }));
  promises.push(setSettings(SETTINGS_KEY_DATA_ENABLED, aEnabled));

  return Promise.all(promises);
}












function setupDataCallAndWait(aNetworkType) {
  log("setupDataCallAndWait: " + aNetworkType);

  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, aNetworkType,
         "subject.type should be " + aNetworkType);
      is(aSubject.state, Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED,
         "subject.state should be CONNECTED");
    }));
  promises.push(radioInterface.setupDataCallByType(aNetworkType));

  return Promise.all(promises);
}












function deactivateDataCallAndWait(aNetworkType) {
  log("deactivateDataCallAndWait: " + aNetworkType);

  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, aNetworkType,
         "subject.type should be " + aNetworkType);
      is(aSubject.state, Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED,
         "subject.state should be DISCONNECTED");
    }));
  promises.push(radioInterface.deactivateDataCallByType(aNetworkType));

  return Promise.all(promises);
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
    .then(aTestCaseMain)
    .then(finish, function(aException) {
      ok(false, "promise rejects during test: " + aException);
      finish();
    });
}
