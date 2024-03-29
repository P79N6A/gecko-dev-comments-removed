


subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

add_test(function test_setCallWaiting_success() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL.setCallWaiting = function fakeSetCallWaiting(options) {
    context.RIL[REQUEST_SET_CALL_WAITING](0, {});
  };

  context.RIL.setCallWaiting({
    enabled: true
  });

  let postedMessage = workerHelper.postedMessage;

  equal(postedMessage.errorMsg, undefined);

  run_next_test();
});

add_test(function test_setCallWaiting_generic_failure() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.RIL.setCallWaiting = function fakeSetCallWaiting(options) {
    context.RIL[REQUEST_SET_CALL_WAITING](0, {
      errorMsg: GECKO_ERROR_GENERIC_FAILURE
    });
  };

  context.RIL.setCallWaiting({
    enabled: true
  });

  let postedMessage = workerHelper.postedMessage;

  equal(postedMessage.errorMsg, GECKO_ERROR_GENERIC_FAILURE);

  run_next_test();
});

add_test(function test_queryCallWaiting_success_enabled_true() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.Buf.readInt32 = function fakeReadUint32() {
    return context.Buf.int32Array.pop();
  };

  context.RIL.queryCallWaiting = function fakeQueryCallWaiting(options) {
    context.Buf.int32Array = [
      1,  
      1,  
      2   
    ];
    context.RIL[REQUEST_QUERY_CALL_WAITING](1, {});
  };

  context.RIL.queryCallWaiting({});

  let postedMessage = workerHelper.postedMessage;

  equal(postedMessage.errorMsg, undefined);
  equal(postedMessage.serviceClass, 1);
  run_next_test();
});

add_test(function test_queryCallWaiting_success_enabled_false() {
  let workerHelper = newInterceptWorker();
  let worker = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];

  context.Buf.readInt32 = function fakeReadUint32() {
    return context.Buf.int32Array.pop();
  };

  context.RIL.queryCallWaiting = function fakeQueryCallWaiting(options) {
    context.Buf.int32Array = [
      1,  
      0,  
      2   
    ];
    context.RIL[REQUEST_QUERY_CALL_WAITING](1, {});
  };

  context.RIL.queryCallWaiting({});

  let postedMessage = workerHelper.postedMessage;

  equal(postedMessage.errorMsg, undefined);
  equal(postedMessage.serviceClass, ICC_SERVICE_CLASS_NONE);
  run_next_test();
});
