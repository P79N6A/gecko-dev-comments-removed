


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function verifyVoiceCellLocationInfo(aLac, aCid) {
  let cell = mobileConnection.voice.cell;
  ok(cell, "location available");

  
  
  is(cell.gsmLocationAreaCode, aLac, "check voice.cell.gsmLocationAreaCode");
  is(cell.gsmCellId, aCid, "check voice.cell.gsmCellId");
  is(cell.cdmaBaseStationId, -1, "check voice.cell.cdmaBaseStationId");
  is(cell.cdmaBaseStationLatitude, -2147483648,
     "check voice.cell.cdmaBaseStationLatitude");
  is(cell.cdmaBaseStationLongitude, -2147483648,
     "check voice.cell.cdmaBaseStationLongitude");
  is(cell.cdmaSystemId, -1, "check voice.cell.cdmaSystemId");
  is(cell.cdmaNetworkId, -1, "check voice.cell.cdmaNetworkId");
}


function testVoiceCellLocationUpdate(aLac, aCid) {
  
  log("Test cell location with lac=" + aLac + " and cid=" + aCid);

  let promises = [];
  promises.push(waitForManagerEvent("voicechange"));
  promises.push(setEmulatorGsmLocation(aLac, aCid));
  return Promise.all(promises)
    .then(() => verifyVoiceCellLocationInfo(aLac, aCid));
}

startTestCommon(function() {
  return getEmulatorGsmLocation()
    .then(function(aResult) {
      log("Test initial voice location info");
      verifyVoiceCellLocationInfo(aResult.lac, aResult.cid);

      return Promise.resolve()
        .then(() => testVoiceCellLocationUpdate(100, 100))
        .then(() => testVoiceCellLocationUpdate(2000, 2000))

        
        .then(() => testVoiceCellLocationUpdate(aResult.lac, aResult.cid));
    });
});
