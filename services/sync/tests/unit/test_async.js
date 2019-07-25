function run_test() {
  var async = loadInSandbox("resource://weave/async.js");
  var callbackQueue = [];

  Function.prototype.async = async.Async.sugar;

  async.makeTimer = function fake_makeTimer(cb) {
    
    
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
