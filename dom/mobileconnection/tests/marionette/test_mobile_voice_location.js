


MARIONETTE_TIMEOUT = 20000;
MARIONETTE_HEAD_JS = "mobile_header.js";

function setEmulatorGsmLocation(lac, cid) {
  emulatorHelper.sendCommand("gsm location " + lac + " " + cid);
}

function waitForVoiceChangeEvent(callback) {
  mobileConnection.addEventListener("voicechange", function onvoicechange() {
    mobileConnection.removeEventListener("voicechange", onvoicechange);

    if (callback && typeof callback === "function") {
      callback();
    }
  });
}


taskHelper.push(function testInitialVoiceCellLocationInfo() {
  log("Test initial voice location info");

  let cell = mobileConnection.voice.cell;
  ok(cell, "location available");
  
  
  is(cell.gsmLocationAreaCode, 65535, "check voice.cell.gsmLocationAreaCode");
  is(cell.gsmCellId, 268435455, "check voice.cell.gsmCellId");
  is(cell.cdmaBaseStationId, -1, "check voice.cell.cdmaBaseStationId");
  is(cell.cdmaBaseStationLatitude, -2147483648,
     "check voice.cell.cdmaBaseStationLatitude");
  is(cell.cdmaBaseStationLongitude, -2147483648,
     "check voice.cell.cdmaBaseStationLongitude");
  is(cell.cdmaSystemId, -1, "check voice.cell.cdmaSystemId");
  is(cell.cdmaNetworkId, -1, "check voice.cell.cdmaNetworkId");

  taskHelper.runNext();
});


taskHelper.push(function testVoiceCellLocationUpdate() {
  
  function doTestVoiceCellLocation(lac, cid, callback) {
    log("Test cell location with lac=" + lac + " and cid=" + cid);

    waitForVoiceChangeEvent(function() {
      let cell = mobileConnection.voice.cell;
      is(cell.gsmLocationAreaCode, lac, "check voice.cell.gsmLocationAreaCode");
      is(cell.gsmCellId, cid, "check voice.cell.gsmCellId");
      
      is(cell.cdmaBaseStationId, -1, "check voice.cell.cdmaBaseStationId");
      is(cell.cdmaBaseStationLatitude, -2147483648,
         "check voice.cell.cdmaBaseStationLatitude");
      is(cell.cdmaBaseStationLongitude, -2147483648,
         "check voice.cell.cdmaBaseStationLongitude");
      is(cell.cdmaSystemId, -1, "check voice.cell.cdmaSystemId");
      is(cell.cdmaNetworkId, -1, "check voice.cell.cdmaNetworkId");

      if (callback && typeof callback === "function") {
        callback();
      }
    });

    setEmulatorGsmLocation(lac, cid);
  }

  let testData = [
    {lac: 100, cid: 100},
    {lac: 2000, cid: 2000},
    
    {lac: 65535, cid: 268435455}
  ];

  
  (function do_call() {
    let next = testData.shift();
    if (!next) {
      taskHelper.runNext();
      return;
    }
    doTestVoiceCellLocation(next.lac, next.cid, do_call);
  })();
});


taskHelper.runNext();
