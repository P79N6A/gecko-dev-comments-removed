









"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/LogCapture.jsm");
Cu.import("resource://gre/modules/LogShake.jsm");

const EVENTS_PER_SECOND = 6.25;
const GRAVITY = 9.8;






function sendDeviceMotionEvent(x, y, z) {
  let event = {
    type: "devicemotion",
    accelerationIncludingGravity: {
      x: x,
      y: y,
      z: z
    }
  };
  LogShake.handleEvent(event);
}





function sendScreenChangeEvent(screenEnabled) {
  let event = {
    type: "screenchange",
    detail: {
      screenEnabled: screenEnabled
    }
  };
  LogShake.handleEvent(event);
}






function mockReadLogFile() {
  let readLocations = [];

  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    return null; 
  };

  
  return readLocations;
}




function sendSustainedShake() {
  
  for (let i = 0; i < 2 * EVENTS_PER_SECOND; i++) {
    sendDeviceMotionEvent(0, 2 * GRAVITY, 2 * GRAVITY);
  }

}

add_test(function test_do_log_capture_after_shaking() {
  
  LogShake.init();

  let readLocations = mockReadLogFile();

  sendSustainedShake();

  ok(readLocations.length > 0,
      "LogShake should attempt to read at least one log");

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_nothing_when_resting() {
  
  LogShake.init();

  let readLocations = mockReadLogFile();

  
  for (let i = 0; i < 2 * EVENTS_PER_SECOND; i++) {
    sendDeviceMotionEvent(0, GRAVITY, GRAVITY);
  }

  ok(readLocations.length === 0,
      "LogShake should not read any logs");

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_nothing_when_disabled() {
  
  LogShake.uninit();

  let readLocations = mockReadLogFile();

  
  sendSustainedShake();

  ok(readLocations.length === 0,
      "LogShake should not read any logs");

  run_next_test();
});

add_test(function test_do_nothing_when_screen_off() {
  
  LogShake.init();

  
  sendScreenChangeEvent(false);

  let readLocations = mockReadLogFile();

  
  sendSustainedShake();

  ok(readLocations.length === 0,
      "LogShake should not read any logs");

  
  sendScreenChangeEvent(true);

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_log_capture_resilient_readLogFile() {
  
  LogShake.init();

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    throw new Error("Exception during readLogFile for: " + loc);
  };

  
  sendSustainedShake();

  ok(readLocations.length > 0,
      "LogShake should attempt to read at least one log");

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_log_capture_resilient_parseLog() {
  
  LogShake.init();

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    LogShake.LOGS_WITH_PARSERS[loc] = function() {
      throw new Error("Exception during LogParser for: " + loc);
    };
    return null;
  };

  
  sendSustainedShake();

  ok(readLocations.length > 0,
      "LogShake should attempt to read at least one log");

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_nothing_when_dropped() {
  
  LogShake.init();

  let readLocations = mockReadLogFile();

  
  

  for (let i = 0; i < 10 * EVENTS_PER_SECOND; i++) {
    
    sendDeviceMotionEvent(0, 0, GRAVITY);
    
    sendDeviceMotionEvent(0, 2 * GRAVITY, 2 * GRAVITY);
  }

  ok(readLocations.length === 0,
      "LogShake should not read any logs");

  LogShake.uninit();
  run_next_test();
});

function run_test() {
  run_next_test();
}
