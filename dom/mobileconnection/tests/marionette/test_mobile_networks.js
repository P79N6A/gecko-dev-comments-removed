



MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function isHomeNetwork(network) {
  is(network.longName, "Android");
  is(network.shortName, "Android");
  is(network.mcc, "310");
  is(network.mnc, "260");
}

function isRoamingNetwork(network) {
  is(network.longName, "TelKila");
  is(network.shortName, "TelKila");
  is(network.mcc, "310");
  is(network.mnc, "295");
}

function testConnectionInfo() {
  log("Validate initial states");
  let voice = mobileConnection.voice;
  is(voice.connected, true);
  is(voice.state, "registered");
  is(voice.emergencyCallsOnly, false);
  is(voice.roaming, false);
  isHomeNetwork(voice.network);

  let data = mobileConnection.data;
  
  
  is(data.state, "registered");
  is(data.emergencyCallsOnly, false);
  is(data.roaming, false);
  isHomeNetwork(data.network);
}

function testGetNetworks() {
  log("Enumerating available networks");
  return getNetworks()
    .then(function resolve(aNetworks) {
      
      
      
      is(aNetworks.length, 2);

      let network1 = aNetworks[0];
      isHomeNetwork(network1);
      is(network1.state, "available");

      let network2 = aNetworks[1];
      isRoamingNetwork(network2);
      is(network2.state, "available");

      return aNetworks;
    });
}

function testSelectNetwork(aNetwork, aValidator) {
  log("Selecting network '" + aNetwork.longName + "' manually");
  isnot(aNetwork.longName, mobileConnection.voice.network.longName,
        "aNetwork.longName");

  return selectNetworkAndWait(aNetwork)
    .then(function() {
      is(mobileConnection.networkSelectionMode, "manual",
         "mobileConnection.networkSelectionMode");
      is(mobileConnection.voice.network.longName, aNetwork.longName,
         "mobileConnection.voice.network.longName");

      aValidator(mobileConnection.voice.network);
    });
}

function testSelectNetworkAutomatically(aHomeNetwork, aValidator) {
  log("Selecting network '" + aHomeNetwork.longName + "' automatically");
  isnot(aHomeNetwork.longName, mobileConnection.voice.network.longName,
        "aHomeNetwork.longName");

  return selectNetworkAutomaticallyAndWait()
    .then(function() {
      is(mobileConnection.networkSelectionMode, "automatic",
         "mobileConnection.networkSelectionMode");
      is(mobileConnection.voice.network.longName, aHomeNetwork.longName,
         "mobileConnection.voice.network.longName");

      aValidator(mobileConnection.voice.network);
    });
}

function throwsException(fn) {
  try {
    fn();
    ok(false, "function did not throw an exception: " + fn);
  } catch (e) {
    ok(true, "function successfully caught exception: " + e);
  }
}

function testSelectNetworkErrors(aNetworkToSelect, aAnotherNetwork) {
  throwsException(() => mobileConnection.selectNetwork(null));
  throwsException(() => mobileConnection.selectNetwork({}));

  isnot(aNetworkToSelect.longName, mobileConnection.voice.network.longName,
        "aNetworkToSelect.longName");

  let promises = [];
  promises.push(selectNetworkAndWait(aNetworkToSelect));
  
  
  promises.push(selectNetwork(aAnotherNetwork)
    .then(function resolve() {
      ok(false, "should not success");
    }, function reject(aError) {
      is(aError.name, "AlreadySelectingANetwork", "got an error");
    }));

  return Promise.all(promises);
}

function testSelectExistingNetworkManual(aNetwork) {
  
  

  log("Selecting '" + aNetwork.longName + "' manually (should already be selected)");
  is(aNetwork.longName, mobileConnection.voice.network.longName,
     "aNetwork.longName");

  function voiceChange() {
    let network = mobileConnection.voice.network;
    if (network.longName !== aNetwork.longName) {
      ok(false, "voicechange event emitted while selecting existing '" +
                aNetwork.longName + "' manually");
    }
  }

  mobileConnection.addEventListener("voicechange", voiceChange);

  return selectNetwork(aNetwork)
    .then(function resolve() {
      let deferred = Promise.defer();

      
      setTimeout(function() {
        mobileConnection.removeEventListener("voicechange", voiceChange);
        deferred.resolve();
      }, 3000);

      return deferred.promise;
    }, function reject() {
      mobileConnection.removeEventListener("voicechange", voiceChange);
      ok(false, "selectNetwork fails");
    });
}

function testSelectExistingNetworkAuto(aHomeNetwork) {
  
  log("Selecting '" + aHomeNetwork.longName + "' automatically (should already be selected)");
  is(aHomeNetwork.longName, mobileConnection.voice.network.longName,
     "aHomeNetwork.longName");

  function voiceChange() {
    let network = mobileConnection.voice.network;
    if (network.longName !== aHomeNetwork.longName) {
      ok(false, "voicechange event emitted while selecting existing '" +
                aHomeNetwork.longName + "' automatically");
    }
  }

  mobileConnection.addEventListener("voicechange", voiceChange);

  return selectNetworkAutomatically()
    .then(function resolve() {
      let deferred = Promise.defer();

      
      setTimeout(function() {
        mobileConnection.removeEventListener("voicechange", voiceChange);
        deferred.resolve();
      }, 3000);

      return deferred.promise;
    }, function reject() {
      mobileConnection.removeEventListener("voicechange", voiceChange);
      ok(false, "selectNetwork fails");
    });
}

startTestCommon(function() {
  let promise = Promise.resolve();
  if (mobileConnection.networkSelectionMode != "automatic") {
    promise = promise.then(selectNetworkAutomatically);
  }

  return promise
    .then(() => testConnectionInfo())
    .then(() => testGetNetworks())
    .then(function(aNetworks) {
      let homeNetwork = aNetworks[0],
          roamingNetwork = aNetworks[1];

      
      
      return testSelectNetwork(roamingNetwork, isRoamingNetwork)

        
        .then(() => testSelectNetworkAutomatically(homeNetwork, isHomeNetwork))

        
        .then(() => testSelectNetworkErrors(roamingNetwork, homeNetwork))

        
        .then(() => testSelectExistingNetworkManual(roamingNetwork))

        
        .then(() => selectNetworkAutomaticallyAndWait())
        .then(() => testSelectExistingNetworkAuto(homeNetwork));
    });
});
