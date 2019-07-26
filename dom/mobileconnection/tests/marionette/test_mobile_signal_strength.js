


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "mobile_header.js";


function setEmulatorGsmSignalStrength(rssi) {
  emulatorHelper.sendCommand("gsm signal " + rssi);
}


function setEmulatorLteSignalStrength(rxlev, rsrp, rssnr) {
  let lteSignal = rxlev + " " + rsrp + " " + rssnr;
  emulatorHelper.sendCommand("gsm lte_signal " + lteSignal);
}

function waitForVoiceChangeEvent(callback) {
  mobileConnection.addEventListener("voicechange", function onvoicechange() {
    mobileConnection.removeEventListener("voicechange", onvoicechange);

    if (callback && typeof callback === "function") {
      callback();
    }
  });
}


taskHelper.push(function testInitialSignalStrengthInfo() {
  log("Test initial signal strength info");

  let voice = mobileConnection.voice;
  
  is(voice.signalStrength, -99, "check voice.signalStrength");
  is(voice.relSignalStrength, 44, "check voice.relSignalStrength");

  taskHelper.runNext();
});


taskHelper.push(function testLteSignalStrength() {
  
  function doTestLteSignalStrength(input, expect, callback) {
    log("Test LTE signal info with data : " + JSON.stringify(input));

    waitForVoiceChangeEvent(function() {
      let voice = mobileConnection.voice;
      is(voice.signalStrength, expect.signalStrength,
         "check voice.signalStrength");
      is(voice.relSignalStrength, expect.relSignalStrength,
         "check voice.relSignalStrength");

      if (callback && typeof callback === "function") {
        callback();
      }
    });

    setEmulatorLteSignalStrength(input.rxlev, input.rsrp, input.rssnr);
  }

  let testData = [
    
    {input: {
      rxlev: 99,
      rsrp: 65535,
      rssnr: 65535},
     expect: {
      signalStrength: null,
      relSignalStrength: null}
    },
    
    {input: {
      rxlev: 63,
      rsrp: 65535,
      rssnr: 65535},
     expect: {
      signalStrength: -48,
      relSignalStrength: 100}
    },
    
    {input: {
      rxlev: 12,
      rsrp: 65535,
      rssnr: 65535},
     expect: {
      signalStrength: -99,
      relSignalStrength: 100}
    },
    
    {input: {
      rxlev: 0,
      rsrp: 65535,
      rssnr: 65535},
     expect: {
      signalStrength: -111,
      relSignalStrength: 0}
    }
  ];

  
  (function do_call() {
    let next = testData.shift();
    if (!next) {
      taskHelper.runNext();
      return;
    }
    doTestLteSignalStrength(next.input, next.expect, do_call);
  })();
});


taskHelper.push(function testResetSignalStrengthInfo() {
  
  function doResetSignalStrength(rssi) {
    waitForVoiceChangeEvent(function() {
      let voice = mobileConnection.voice;
      is(voice.signalStrength, -99, "check voice.signalStrength");
      is(voice.relSignalStrength, 44, "check voice.relSignalStrength");

      taskHelper.runNext();
    });

    setEmulatorGsmSignalStrength(rssi);
  }

  
  
  doResetSignalStrength(7);
});


taskHelper.runNext();
