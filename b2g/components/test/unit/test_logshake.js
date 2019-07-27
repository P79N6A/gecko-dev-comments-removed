









'use strict';

const Cu = Components.utils;

Cu.import('resource://gre/modules/LogCapture.jsm');
Cu.import('resource://gre/modules/LogShake.jsm');




function sendDeviceMotionEvent(x, y, z) {
  let event = {
    type: 'devicemotion',
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
    type: 'screenchange',
    detail: {
      screenEnabled: screenEnabled
    }
  };
  LogShake.handleEvent(event);
}

function debug(msg) {
  var timestamp = Date.now();
  dump('LogShake: ' + timestamp + ': ' + msg);
}

add_test(function test_do_log_capture_after_shaking() {
  
  LogShake.init();

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    return null; 
  };

  
  sendDeviceMotionEvent(9001, 9001, 9001);

  ok(readLocations.length > 0,
      'LogShake should attempt to read at least one log');

  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_nothing_when_resting() {
  
  LogShake.init();

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    return null; 
  };

  
  sendDeviceMotionEvent(0, 9.8, 9.8);

  ok(readLocations.length === 0,
      'LogShake should not read any logs');

  debug('test_do_nothing_when_resting: stop');
  LogShake.uninit();
  run_next_test();
});

add_test(function test_do_nothing_when_disabled() {
  debug('test_do_nothing_when_disabled: start');
  
  LogShake.uninit();

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    return null; 
  };

  
  sendDeviceMotionEvent(0, 9001, 9001);

  ok(readLocations.length === 0,
      'LogShake should not read any logs');

  run_next_test();
});

add_test(function test_do_nothing_when_screen_off() {
  
  LogShake.init();


  
  sendScreenChangeEvent(false);

  let readLocations = [];
  LogCapture.readLogFile = function(loc) {
    readLocations.push(loc);
    return null; 
  };

  
  sendDeviceMotionEvent(0, 9001, 9001);

  ok(readLocations.length === 0,
      'LogShake should not read any logs');

  
  sendScreenChangeEvent(true);

  LogShake.uninit();
  run_next_test();
});

function run_test() {
  debug('Starting');
  run_next_test();
}
