


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

function _getWorker() {
  let _postedMessage;
  let _worker = newWorker({
    postRILMessage: function fakePostRILMessage(data) {
    },
    postMessage: function fakePostMessage(message) {
      _postedMessage = message;
    }
  });
  return {
    get postedMessage() {
      return _postedMessage;
    },
    get worker() {
      return _worker;
    }
  };
}

var timeoutCallback = null;
var timeoutDelayMs = 0;
var TIMER_ID = 1234;



function setTimeout(callback, timeoutMs) {
  timeoutCallback = callback;
  timeoutDelayMs = timeoutMs;
  do_check_eq(timeoutMs, 300000);  
  return TIMER_ID;
}

function clearTimeout(timeoutId) {
  do_check_eq(timeoutId, TIMER_ID);
  timeoutCallback = null;
}

function fireTimeout() {
  do_check_neq(timeoutCallback, null);
  if (timeoutCallback) {
    timeoutCallback();
    timeoutCallback = null;
  }
}

add_test(function test_enter_emergencyCbMode() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;

  worker.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  let postedMessage = workerHelper.postedMessage;

  
  do_check_eq(worker.RIL._isInEmergencyCbMode, true);

  
  do_check_eq(postedMessage.rilMessageType, "emergencyCbModeChange");
  do_check_eq(postedMessage.active, true);
  do_check_eq(postedMessage.timeoutMs, 300000);

  
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  run_next_test();
});

add_test(function test_exit_emergencyCbMode() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;

  worker.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  worker.RIL[UNSOLICITED_EXIT_EMERGENCY_CALLBACK_MODE]();
  let postedMessage = workerHelper.postedMessage;

  
  do_check_eq(worker.RIL._isInEmergencyCbMode, false);

  
  do_check_eq(postedMessage.rilMessageType, "emergencyCbModeChange");
  do_check_eq(postedMessage.active, false);

  
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, null);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_when_timeout() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;
  do_register_cleanup(function() { cleanUp(worker); });

  worker.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(worker.RIL._isInEmergencyCbMode, true);
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  worker.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  
  fireTimeout();

  
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_when_dial() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;

  worker.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(worker.RIL._isInEmergencyCbMode, true);
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  worker.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  
  worker.RIL.dial({number: "0912345678",
                  isDialEmergency: false});

  
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_explicitly() {
  let workerHelper = _getWorker();
  let worker = workerHelper.worker;

  worker.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(worker.RIL._isInEmergencyCbMode, true);
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  worker.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  worker.RIL.handleChromeMessage({rilMessageType: "exitEmergencyCbMode"});
  worker.RIL[REQUEST_EXIT_EMERGENCY_CALLBACK_MODE](1, {
    rilMessageType: "exitEmergencyCbMode",
    rilRequestError: ERROR_SUCCESS
  });
  let postedMessage = workerHelper.postedMessage;

  
  do_check_eq(worker.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  
  do_check_eq(postedMessage.rilMessageType, "exitEmergencyCbMode");

  run_next_test();
});

