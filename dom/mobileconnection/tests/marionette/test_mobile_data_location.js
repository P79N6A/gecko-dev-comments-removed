


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function verifyDataCellLocationInfo(aLac, aCid) {
  let cell = mobileConnection.data.cell;
  ok(cell, "location available");

  is(cell.gsmLocationAreaCode, aLac, "data.cell.gsmLocationAreaCode");
  is(cell.gsmCellId, aCid, "data.cell.gsmCellId");
  is(cell.cdmaBaseStationId, -1, "data.cell.cdmaBaseStationId");
  is(cell.cdmaBaseStationLatitude, -2147483648,
     "data.cell.cdmaBaseStationLatitude");
  is(cell.cdmaBaseStationLongitude, -2147483648,
     "data.cell.cdmaBaseStationLongitude");
  is(cell.cdmaSystemId, -1, "data.cell.cdmaSystemId");
  is(cell.cdmaNetworkId, -1, "data.cell.cdmaNetworkId");
}


function testDataCellLocationUpdate(aLac, aCid) {
  
  log("Test cell location with lac=" + aLac + " and cid=" + aCid);

  return setEmulatorGsmLocationAndWait(aLac, aCid, false, true)
    .then(() => verifyDataCellLocationInfo(aLac, aCid));
}

startTestCommon(function() {
  return getEmulatorGsmLocation()
    .then(function(aResult) {
      log("Test initial data location info");
      verifyDataCellLocationInfo(aResult.lac, aResult.cid);

      return Promise.resolve()
        .then(() => testDataCellLocationUpdate(100, 100))
        .then(() => testDataCellLocationUpdate(2000, 2000))

        
        .then(() => testDataCellLocationUpdate(aResult.lac, aResult.cid));
    });
});
