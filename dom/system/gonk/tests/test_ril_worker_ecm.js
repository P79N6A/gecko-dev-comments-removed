


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

var timeoutCallback = null;
var timeoutDelayMs = 0;
const TIMER_ID = 1234;
const TIMEOUT_VALUE = 300000;  



function setTimeout(callback, timeoutMs) {
  timeoutCallback = callback;
  timeoutDelayMs = timeoutMs;
  do_check_eq(timeoutMs, TIMEOUT_VALUE);
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
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  
  for (let i = 0; i < 2; ++i) {
    context.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
    let postedMessage = workerHelper.postedMessage;

    
    do_check_eq(context.RIL._isInEmergencyCbMode, true);

    
    do_check_eq(postedMessage.rilMessageType, "emergencyCbModeChange");
    do_check_eq(postedMessage.active, true);
    do_check_eq(postedMessage.timeoutMs, TIMEOUT_VALUE);

    
    do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);
  }

  run_next_test();
});

add_test(function test_exit_emergencyCbMode() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  context.RIL[UNSOLICITED_EXIT_EMERGENCY_CALLBACK_MODE]();
  let postedMessage = workerHelper.postedMessage;

  
  do_check_eq(context.RIL._isInEmergencyCbMode, false);

  
  do_check_eq(postedMessage.rilMessageType, "emergencyCbModeChange");
  do_check_eq(postedMessage.active, false);

  
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, null);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_when_timeout() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(context.RIL._isInEmergencyCbMode, true);
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  context.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  
  fireTimeout();

  
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_when_dial() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(context.RIL._isInEmergencyCbMode, true);
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  context.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  
  context.RIL.dialNonEmergencyNumber({number: "0912345678",
                                      isEmergency: false,
                                      isDialEmergency: false});

  
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  run_next_test();
});

add_test(function test_request_exit_emergencyCbMode_explicitly() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE]();
  do_check_eq(context.RIL._isInEmergencyCbMode, true);
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, TIMER_ID);

  let parcelTypes = [];
  context.Buf.newParcel = function(type, options) {
    parcelTypes.push(type);
  };

  context.RIL.handleChromeMessage({rilMessageType: "exitEmergencyCbMode"});
  context.RIL[REQUEST_EXIT_EMERGENCY_CALLBACK_MODE](1, {
    rilMessageType: "exitEmergencyCbMode",
    rilRequestError: ERROR_SUCCESS
  });
  let postedMessage = workerHelper.postedMessage;

  
  do_check_eq(context.RIL._exitEmergencyCbModeTimeoutID, null);

  
  do_check_neq(parcelTypes.indexOf(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE), -1);

  
  do_check_eq(postedMessage.rilMessageType, "exitEmergencyCbMode");

  run_next_test();
});

