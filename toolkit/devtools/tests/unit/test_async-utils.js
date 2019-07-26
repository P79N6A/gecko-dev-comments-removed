





const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});



Object.defineProperty(this, "Promise", {
  value: Cu.import("resource://gre/modules/Promise.jsm", {}).Promise,
  writable: false, configurable: false
});
const {require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const {async, asyncOnce, promiseInvoke, promiseCall} = require("devtools/async-utils");

function run_test() {
  do_test_pending();
  Task.spawn(function*() {
    for (let helper of [async, asyncOnce]) {
      yield test_async_args(helper);
      yield test_async_return(helper);
      yield test_async_throw(helper);
    }
    yield test_async_once();
    yield test_async_invoke();
    do_test_finished();
  }).then(null, error => {
    do_throw(error);
  });
}


function test_async_args(async) {
  let obj = {
    method: async(function*(a, b) {
      do_check_eq(this, obj);
      do_check_eq(a, "foo");
      do_check_eq(b, "bar");
    })
  };

  return obj.method("foo", "bar");
}



function test_async_return(async) {
  let obj = {
    method: async(function*(a, b) {
      return a + b;
    })
  };

  return obj.method("foo", "bar").then(ret => {
    do_check_eq(ret, "foobar");
  });
}


function test_async_throw(async) {
  let obj = {
    method: async(function*() {
      throw "boom";
    })
  };

  return obj.method().then(null, error => {
    do_check_eq(error, "boom");
  });
}



function test_async_once() {
  let counter = 0;

  function Foo() {}
  Foo.prototype = {
    ran: false,
    method: asyncOnce(function*() {
      yield Promise.resolve();
      if (this.ran) {
        do_throw("asyncOnce function unexpectedly ran twice on the same object");
      }
      this.ran = true;
      return counter++;
    })
  };

  let foo1 = new Foo();
  let foo2 = new Foo();
  let p1 = foo1.method();
  let p2 = foo2.method();

  do_check_neq(p1, p2);

  let p3 = foo1.method();
  do_check_eq(p1, p3);
  do_check_false(foo1.ran);

  let p4 = foo2.method();
  do_check_eq(p2, p4);
  do_check_false(foo2.ran);

  return p1.then(ret => {
    do_check_true(foo1.ran);
    do_check_eq(ret, 0);
    return p2;
  }).then(ret => {
    do_check_true(foo2.ran);
    do_check_eq(ret, 1);
  });
}


function test_async_invoke() {
  return Task.spawn(function*() {
    function func(a, b, expectedThis, callback) {
      "use strict";
      do_check_eq(a, "foo");
      do_check_eq(b, "bar");
      do_check_eq(this, expectedThis);
      callback(a + b);
    }

    
    let callResult = yield promiseCall(func, "foo", "bar", undefined);
    do_check_eq(callResult, "foobar");


    
    let obj = { method: func };
    let invokeResult = yield promiseInvoke(obj, obj.method, "foo", "bar", obj);
    do_check_eq(invokeResult, "foobar");


    
    function multipleResults(callback) {
      callback("foo", "bar");
    }

    let results = yield promiseCall(multipleResults);
    do_check_eq(results.length, 2);
    do_check_eq(results[0], "foo");
    do_check_eq(results[1], "bar");


    
    function thrower() {
      throw "boom";
    }

    yield promiseCall(thrower).then(null, error => {
      do_check_eq(error, "boom");
    });
  });
}
