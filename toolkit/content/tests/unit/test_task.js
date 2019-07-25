









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/promise/core.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");





function promiseResolvedLater(aValue) {
  let deferred = Promise.defer();
  Services.tm.mainThread.dispatch(function () deferred.resolve(aValue),
                                  Ci.nsIThread.DISPATCH_NORMAL);
  return deferred.promise;
}




function run_test()
{
  run_next_test();
}

add_test(function test_normal()
{
  Task.spawn(function () {
    let result = yield Promise.resolve("Value");
    for (let i = 0; i < 3; i++) {
      result += yield promiseResolvedLater("!");
    }
    throw new Task.Result("Task result: " + result);
  }).then(function (result) {
    do_check_eq("Task result: Value!!!", result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});

add_test(function test_exceptions()
{
  Task.spawn(function () {
    try {
      yield Promise.reject("Rejection result by promise.");
      do_throw("Exception expected because the promise was rejected.");
    } catch (ex) {
      
      do_check_eq("Rejection result by promise.", ex);
    }
    throw new Error("Exception uncaught by task.");
  }).then(function (result) {
    do_throw("Unexpected success!");
  }, function (ex) {
    do_check_eq("Exception uncaught by task.", ex.message);
    run_next_test();
  });
});

add_test(function test_recursion()
{
  function task_fibonacci(n) {
    throw new Task.Result(n < 2 ? n : (yield task_fibonacci(n - 1)) +
                                      (yield task_fibonacci(n - 2)));
  };

  Task.spawn(task_fibonacci(6)).then(function (result) {
    do_check_eq(8, result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});

add_test(function test_spawn_primitive()
{
  function fibonacci(n) {
    return n < 2 ? n : fibonacci(n - 1) + fibonacci(n - 2);
  };

  
  Task.spawn(fibonacci(6)).then(function (result) {
    do_check_eq(8, result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});

add_test(function test_spawn_function()
{
  Task.spawn(function () {
    return "This is not a generator.";
  }).then(function (result) {
    do_check_eq("This is not a generator.", result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});

add_test(function test_yielded_undefined()
{
  Task.spawn(function () {
    yield;
    throw new Task.Result("We continued correctly.");
  }).then(function (result) {
    do_check_eq("We continued correctly.", result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});

add_test(function test_yielded_primitive()
{
  Task.spawn(function () {
    throw new Task.Result("Primitive " + (yield "value."));
  }).then(function (result) {
    do_check_eq("Primitive value.", result);
    run_next_test();
  }, function (ex) {
    do_throw("Unexpected error: " + ex);
  });
});
