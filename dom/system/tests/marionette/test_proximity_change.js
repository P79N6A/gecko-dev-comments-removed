


MARIONETTE_TIMEOUT = 10000;

let receivedEvent = false;
let expectedEvent;

function enableProximityListener() {
  
  log("Enabling 'deviceproximity' event listener.");

  
  
  
  
  expectedEvent = new DeviceProximityEvent("deviceproximity",
      {value:1, min:0, max:1});

  window.addEventListener('deviceproximity', listener);
  log("Waiting for device proximity event.");
  waitFor(changeProximity, function() {
    return(receivedEvent);
  });
}

function listener(event) {
  
  log("Received 'deviceproximity' event via listener (value:"
      + event.value + " min:" + event.min + " max:" + event.max + ").");
  
  is(event.value, expectedEvent.value, "value");
  is(event.min, expectedEvent.min, "min");
  is(event.max, expectedEvent.max, "max");
  receivedEvent = true;
}

function changeProximity() {
  
  let newValue = "7:3:15";

  
  
  
  
  expectedEvent = new DeviceProximityEvent("deviceproximity",
       {value:7, min:0, max:1});

  
  window.ondeviceproximity = function(event) {
    log("Received 'ondeviceproximity' event via handler (value:"
        + event.value + " min:" + event.min + " max:"
        + event.max + ").");
    is(event.value, expectedEvent.value, "value");
    is(event.min, expectedEvent.min, "min");
    is(event.max, expectedEvent.max, "max");
    restoreProximity();
  };

  log("Sending emulator command to fake proximity change (" + newValue + ").");
  runEmulatorCmd("sensor set proximity " + newValue, function(result) {
    log("Emulator callback.");
  });
}

function restoreProximity() {
  
  newValue = "1:0:0";
  log("Sending emulator command to restore proximity (" + newValue + ").");
  runEmulatorCmd("sensor set proximity " + newValue, function(result) {
    cleanUp();
  });
}

function cleanUp() {
  
  window.removeEventListener('deviceproximity', listener);
  window.ondeviceproximity = null;
  finish();
}


enableProximityListener();
