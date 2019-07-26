


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

const INITIAL_STATES = {
  state: "registered",
  connected: true,
  emergencyCallsOnly: false,
  roaming: false,
  signalStrength: -99,
  relSignalStrength: 44,

  cell: {
    gsmLocationAreaCode: 0,
    gsmCellId: 0,
    cdmaBaseStationId: -1,
    cdmaBaseStationLatitude: -2147483648,
    cdmaBaseStationLongitude: -2147483648,
    cdmaSystemId: -1,
    cdmaNetworkId: -1,
  }
};

const TEST_DATA = [{
    
    state: "unregistered",
    expected: {
      state: "notSearching",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }
  }, {
    
    state: "searching",
    expected: {
      state: "searching",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }
  }, {
    
    state: "denied",
    expected: {
      state: "denied",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }
  }, {
    
    state: "roaming",
    expected: {
      state: "registered",
      connected: true,
      emergencyCallsOnly: false,
      roaming: true,
      signalStrength: -99,
      relSignalStrength: 44,
      cell: {
        gsmLocationAreaCode: 0,
        gsmCellId: 0
      }
    }
  }, {
    
    state: "home",
    expected: {
      state: "registered",
      connected: true,
      emergencyCallsOnly: false,
      roaming: false,
      signalStrength: -99,
      relSignalStrength: 44,
      cell: {
        gsmLocationAreaCode: 0,
        gsmCellId: 0
      }
    }
  }
];

function compareTo(aPrefix, aFrom, aTo) {
  for (let field in aTo) {
    let fullName = aPrefix + field;

    let lhs = aFrom[field];
    let rhs = aTo[field];
    ok(true, "lhs=" + JSON.stringify(lhs) + ", rhs=" + JSON.stringify(rhs));
    if (typeof rhs !== "object") {
      is(lhs, rhs, fullName);
    } else if (rhs) {
      ok(lhs, fullName);
      compareTo(fullName + ".", lhs, rhs);
    } else {
      is(lhs, null, fullName);
    }
  }
}

function verifyVoiceInfo(aExpected) {
  compareTo("voice.", mobileConnection.voice, aExpected);
}


function testVoiceStateUpdate(aNewState, aExpected) {
  log("Test voice info with state='" + aNewState + "'");

  
  return setEmulatorVoiceDataStateAndWait("voice", aNewState)
    .then(() => verifyVoiceInfo(aExpected));
}

startTestCommon(function() {
  log("Test initial voice connection info");

  verifyVoiceInfo(INITIAL_STATES);

  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let entry = TEST_DATA[i];
    promise =
      promise.then(testVoiceStateUpdate.bind(null, entry.state, entry.expected));
  }

  return promise;
});
