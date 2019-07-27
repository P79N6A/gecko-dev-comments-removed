


MARIONETTE_CONTEXT = "chrome";

const SETTINGS_KEY_DATA_ENABLED = "ril.data.enabled";
const SETTINGS_KEY_DATA_APN_SETTINGS  = "ril.data.apnSettings";

const TOPIC_CONNECTION_STATE_CHANGED = "network-connection-state-changed";
const TOPIC_NETWORK_ACTIVE_CHANGED = "network-active-changed";

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;

let ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
ok(ril, "ril.constructor is " + ril.constructor);

let radioInterface = ril.getRadioInterface(0);
ok(radioInterface, "radioInterface.constructor is " + radioInterface.constrctor);












function wrapDomRequestAsPromise(aRequest) {
  let deferred = Promise.defer();

  ok(aRequest instanceof DOMRequest,
     "aRequest is instanceof " + aRequest.constructor);

  aRequest.addEventListener("success", function(aEvent) {
    deferred.resolve(aEvent);
  });
  aRequest.addEventListener("error", function(aEvent) {
    deferred.reject(aEvent);
  });

  return deferred.promise;
}


















function getSettings(aKey, aAllowError) {
  let request = window.navigator.mozSettings.createLock().get(aKey);
  return wrapDomRequestAsPromise(request)
    .then(function resolve(aEvent) {
      log("getSettings(" + aKey + ") - success");
      return aEvent.target.result[aKey];
    }, function reject(aEvent) {
      ok(aAllowError, "getSettings(" + aKey + ") - error");
    });
}



















function setSettings(aKey, aValue, aAllowError) {
  let settings = {};
  settings[aKey] = aValue;
  let request = window.navigator.mozSettings.createLock().set(settings);
  return wrapDomRequestAsPromise(request)
    .then(function resolve() {
      log("setSettings(" + JSON.stringify(settings) + ") - success");
    }, function reject() {
      ok(aAllowError, "setSettings(" + JSON.stringify(settings) + ") - error");
    });
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

let mobileTypeMapping = {
  "default": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,
  "mms": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS,
  "supl": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_SUPL,
  "ims": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_IMS,
  "dun": Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_DUN
};












function setDataEnabledAndWait(aEnabled) {
  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE,
         "subject.type should be " + Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE);
      is(aSubject.state,
         aEnabled ? Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED
                  : Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED,
         "subject.state should be " + aEnabled ? "CONNECTED" : "DISCONNECTED");
    }));
  promises.push(setSettings(SETTINGS_KEY_DATA_ENABLED, aEnabled));

  return Promise.all(promises);
}












function setupDataCallAndWait(aType) {
  log("setupDataCallAndWait: " + aType);

  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      let networkType = mobileTypeMapping[aType];
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, networkType,
         "subject.type should be " + networkType);
      is(aSubject.state, Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED,
         "subject.state should be CONNECTED");
    }));
  promises.push(radioInterface.setupDataCallByType(aType));

  return Promise.all(promises);
}












function deactivateDataCallAndWait(aType) {
  log("deactivateDataCallAndWait: " + aType);

  let promises = [];
  promises.push(waitForObserverEvent(TOPIC_CONNECTION_STATE_CHANGED)
    .then(function(aSubject) {
      let networkType = mobileTypeMapping[aType];
      ok(aSubject instanceof Ci.nsIRilNetworkInterface,
         "subject should be an instance of nsIRILNetworkInterface");
      is(aSubject.type, networkType,
         "subject.type should be " + networkType);
      is(aSubject.state, Ci.nsINetworkInterface.NETWORK_STATE_DISCONNECTED,
         "subject.state should be DISCONNECTED");
    }));
  promises.push(radioInterface.deactivateDataCallByType(aType));

  return Promise.all(promises);
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
    .then(aTestCaseMain)
    .then(finish, function() {
      ok(false, 'promise rejects during test.');
      finish();
    });
}
