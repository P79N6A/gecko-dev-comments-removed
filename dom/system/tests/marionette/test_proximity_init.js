


MARIONETTE_TIMEOUT = 10000;

function testEventInit() {
  let initValue = 7;
  let initMin = 1;
  let initMax = 30;

  
  log("Verifying DeviceProximityEvent constructor.");
  let event = new DeviceProximityEvent("deviceproximity",
      {value: initValue, min: initMin, max: initMax});
  is(event.type, "deviceproximity", "event type");
  is(event.value, initValue, "value");
  is(event.min, initMin, "min");
  is(event.max, initMax, "max");
  if (event.value !== initValue ||
      event.min !== initMin ||
      event.max !== initMax) {
    log("DeviceProximityEvent not initialized correctly.");
  }

  
  log("Verifying UserProximityEvent constructor.");
  event = new UserProximityEvent("userproximity", {near: true});
  is(event.type, "userproximity", "event type");
  ok(event.near, "Initialization of UserProximityEvent");
  verifyDefaultStatus();
}

function verifyDefaultStatus() {
  let emulatorDone = false;

  
  log("Getting sensor status from emulator.");

  runEmulatorCmd("sensor status", function(result) {
    log("Received sensor status (" + result[4] + ").");
    is(result[4], "proximity: enabled.", "Proximity sensor enabled by default");
    emulatorDone = true;
  });

  
  waitFor(getDefaultProximity, function() {
    return(emulatorDone);
  });
}

function getDefaultProximity() {
  let expected = "1:0:0";

  
  log("Getting device proximity from emulator.");

  runEmulatorCmd("sensor get proximity", function(result) {
    let values = result[0].split(" ")[2];
    log("Received emulator proximity (" + values + ").");
    is(values, "1:0:0", "Default proximity values");
    cleanUp();
  });
}

function cleanUp() {
  finish();
}


testEventInit();
