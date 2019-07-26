


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";


const DEFAULT_RSSI = 7;

const TEST_DATA = [
  
  {
    input: {
      rxlev: 99,
      rsrp: 65535,
      rssnr: 65535
    },
    expect: {
      signalStrength: null,
      relSignalStrength: null
    }
  },
  
  {
    input: {
      rxlev: 63,
      rsrp: 65535,
      rssnr: 65535
    },
    expect: {
      signalStrength: -48,
      relSignalStrength: 100
    }
  },
  
  {
    input: {
      rxlev: 12,
      rsrp: 65535,
      rssnr: 65535
    },
    expect: {
      signalStrength: -99,
      relSignalStrength: 100
    }
  },
  
  {
    input: {
      rxlev: 0,
      rsrp: 65535,
      rssnr: 65535
    },
    expect: {
      signalStrength: -111,
      relSignalStrength: 0
    }
  }
];

function testInitialSignalStrengthInfo() {
  log("Test initial signal strength info");

  let voice = mobileConnection.voice;
  
  is(voice.signalStrength, -99, "check voice.signalStrength");
  is(voice.relSignalStrength, 44, "check voice.relSignalStrength");
}

function testLteSignalStrength(aInput, aExpect) {
  log("Test setting LTE signal strength to " + JSON.stringify(aInput));

  return setEmulatorLteSignalStrengthAndWait(aInput.rxlev, aInput.rsrp, aInput.rssnr)
    .then(() => {
      let voice = mobileConnection.voice;
      is(voice.signalStrength, aExpect.signalStrength,
         "check voice.signalStrength");
      is(voice.relSignalStrength, aExpect.relSignalStrength,
         "check voice.relSignalStrength");
    });
}


startTestCommon(function() {
  
  testInitialSignalStrengthInfo();

  
  let promise = Promise.resolve();
  for (let i = 0; i < TEST_DATA.length; i++) {
    let data = TEST_DATA[i];
    promise = promise.then(() => testLteSignalStrength(data.input,
                                                       data.expect));
  }

  
  return promise.then(() => setEmulatorGsmSignalStrengthAndWait(DEFAULT_RSSI));
});
