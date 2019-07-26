


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "mobile_header.js";

function setEmulatorDataState(state) {
  emulatorHelper.sendCommand("gsm data " + state);
}

function waitForDataChangeEvent(callback) {
  mobileConnection.addEventListener("datachange", function ondatachange() {
    mobileConnection.removeEventListener("datachange", ondatachange);

    if (callback && typeof callback === "function") {
      callback();
    }
  });
}


taskHelper.push(function testInitialDataInfo() {
  log("Test initial data connection info");

  let data = mobileConnection.data;
  
  
  is(data.connected, false, "check data.connected");
  is(data.state, "registered", "check data.state");
  is(data.emergencyCallsOnly, false, "check data.emergencyCallsOnly");
  is(data.roaming, false, "check data.roaming");
  
  is(data.signalStrength, -99, "check data.signalStrength");
  is(data.relSignalStrength, 44, "check data.relSignalStrength");

  let cell = data.cell;
  ok(data.cell, "location available");
  
  
  is(cell.gsmLocationAreaCode, 65535, "check data.cell.gsmLocationAreaCode");
  is(cell.gsmCellId, 268435455, "check data.cell.gsmCellId");
  is(cell.cdmaBaseStationId, -1, "check data.cell.cdmaBaseStationId");
  is(cell.cdmaBaseStationLatitude, -2147483648,
     "check data.cell.cdmaBaseStationLatitude");
  is(cell.cdmaBaseStationLongitude, -2147483648,
     "check data.cell.cdmaBaseStationLongitude");
  is(cell.cdmaSystemId, -1, "check data.cell.cdmaSystemId");
  is(cell.cdmaNetworkId, -1, "check data.cell.cdmaNetworkId");

  taskHelper.runNext();
});


taskHelper.push(function testDataStateUpdate() {
  
  function doTestDataState(state, expect, callback) {
    log("Test data info with state='" + state + "'");

    waitForDataChangeEvent(function() {
      let data = mobileConnection.data;
      is(data.state, expect.state, "check data.state");
      is(data.connected, expect.connected, "check data.connected");
      is(data.emergencyCallsOnly, expect.emergencyCallsOnly,
         "check data.emergencyCallsOnly");
      is(data.roaming, expect.roaming, "check data.roaming");
      is(data.signalStrength, expect.signalStrength,
         "check data.signalStrength");
      is(data.relSignalStrength, expect.relSignalStrength,
         "check data.relSignalStrength");

      let cell = data.cell;
      if (!expect.cell) {
        ok(!cell, "check data.cell");
      } else {
        is(cell.gsmLocationAreaCode, expect.cell.gsmLocationAreaCode,
           "check data.cell.gsmLocationAreaCode");
        is(cell.gsmCellId, expect.cell.gsmCellId, "check data.cell.gsmCellId");
        is(cell.cdmaBaseStationId, -1, "check data.cell.cdmaBaseStationId");
        is(cell.cdmaBaseStationLatitude, -2147483648,
           "check data.cell.cdmaBaseStationLatitude");
        is(cell.cdmaBaseStationLongitude, -2147483648,
           "check data.cell.cdmaBaseStationLongitude");
        is(cell.cdmaSystemId, -1, "check data.cell.cdmaSystemId");
        is(cell.cdmaNetworkId, -1, "check data.cell.cdmaNetworkId");
      }

      if (callback && typeof callback === "function") {
        callback();
      }
    });

    setEmulatorDataState(state);
  }

  let testData = [
    
    {state: "unregistered",
     expect: {
      state: "notSearching",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }},
    
    {state: "searching",
     expect: {
      state: "searching",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }},
    
    {state: "denied",
     expect: {
      state: "denied",
      connected: false,
      emergencyCallsOnly: true,
      roaming: false,
      signalStrength: null,
      relSignalStrength: null,
      cell: null
    }},
    
    
    
    
    {state: "roaming",
     expect: {
      state: "registered",
      connected: false,
      emergencyCallsOnly: false,
      roaming: false,
      signalStrength: -99,
      relSignalStrength: 44,
      cell: {
        gsmLocationAreaCode: 65535,
        gsmCellId: 268435455
    }}},
    
    {state: "home",
     expect: {
      state: "registered",
      connected: false,
      emergencyCallsOnly: false,
      roaming: false,
      signalStrength: -99,
      relSignalStrength: 44,
      cell: {
        gsmLocationAreaCode: 65535,
        gsmCellId: 268435455
    }}}
  ];

  
  (function do_call() {
    let next = testData.shift();
    if (!next) {
      taskHelper.runNext();
      return;
    }
    doTestDataState(next.state, next.expect, do_call);
  })();
});


taskHelper.runNext();
