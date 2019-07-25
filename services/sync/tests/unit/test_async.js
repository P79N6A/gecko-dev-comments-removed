Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

function run_test() {
  var callbackQueue = [];

  Function.prototype.async = Async.sugar;

  Utils.makeTimerForCall = function fake_makeTimerForCall(cb) {
    
    
    callbackQueue.push(cb);
    return "fake nsITimer";
  };

  var onCompleteCalled = false;

  function onComplete() {
    onCompleteCalled = true;
  }

  let timesYielded = 0;

  function testAsyncFunc(x) {
    let self = yield;
    timesYielded++;

    
    do_check_eq(x, 5);

    
    do_check_eq(this.sampleProperty, true);

    
    
    callbackQueue.push(self.cb);
    yield;

    timesYielded++;
    self.done();
  }

  var thisArg = {sampleProperty: true};
  testAsyncFunc.async(thisArg, onComplete, 5);

  do_check_eq(timesYielded, 1);

  let func = callbackQueue.pop();
  do_check_eq(typeof func, "function");
  func();

  do_check_eq(timesYielded, 2);

  func = callbackQueue.pop();
  do_check_eq(typeof func, "function");
  func();

  do_check_eq(callbackQueue.length, 0);
}
