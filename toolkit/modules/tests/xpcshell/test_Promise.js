

"use strict";

Components.utils.import("resource://gre/modules/Promise.jsm");




let run_promise_tests = function run_promise_tests(tests, cb) {
  let loop = function loop(index) {
    if (index >= tests.length) {
      if (cb) {
        cb.call();
      }
      return;
    }
    do_print("Launching test " + (index + 1) + "/" + tests.length);
    let test = tests[index];
    
    let next = function next() {
      do_print("Test " + (index + 1) + "/" + tests.length + " complete");
      do_execute_soon(function() {
        loop(index + 1);
      });
    };
    let result = test();
    result.then(next, next);
  };
  return loop(0);
};

let make_promise_test = function(test) {
  return function runtest() {
    do_print("Test starting: " + test);
    try {
      let result = test();
      if (result && "promise" in result) {
        result = result.promise;
      }
      if (!result || !("then" in result)) {
        let exn;
        try {
          do_throw("Test " + test + " did not return a promise: " + result);
        } catch (x) {
          exn = x;
        }
        return Promise.reject(exn);
      }
      
      result = result.then(
        
        function onResolve() {
          do_print("Test complete: " + test);
        },
        
        function onReject(err) {
          let detail;
          if (err && typeof err == "object" && "stack" in err) {
            detail = err.stack;
          } else {
            detail = "(no stack)";
          }
          do_throw("Test " + test + " rejected with the following reason: "
              + err + detail);
      });
      return result;
    } catch (x) {
      
      do_throw("Error in body of test " + test + ": " + x + " at " + x.stack);
      return Promise.reject();
    }
  };
};




let tests = [];




let observe_failures = function observe_failures(promise) {
  promise.then(null, function onReject(reason) {
    test.do_throw("Observed failure in test " + test + ": " + reason);
  });
};


tests.push(make_promise_test(
  function notification(test) {
    
    const SIZE = 10;
    const RESULT = "this is an arbitrary value";

    
    let expected = SIZE;

    
    let notified = [];

    
    let source = Promise.defer();
    let result = Promise.defer();

    let install_observer = function install_observer(i) {
      observe_failures(source.promise.then(
        function onSuccess(value) {
          do_check_true(!notified[i], "Ensuring that observer is notified at most once");
          notified[i] = true;

          do_check_eq(value, RESULT, "Ensuring that the observed value is correct");
          if (--expected == 0) {
            result.resolve();
          }
        }));
    };

    
    let i;
    for (i = 0; i < SIZE/2; ++i) {
      install_observer(i);
    }

    source.resolve(RESULT);

    
    for(;i < SIZE; ++i) {
      install_observer(i);
    }

    return result;
  }));


tests.push(
  make_promise_test(function then_returns_before_callbacks(test) {
    let deferred = Promise.defer();
    let promise = deferred.promise;

    let order = 0;

    promise.then(
      function onResolve() {
        do_check_eq(order, 0);
        order++;
      }
    );

    promise.then(
      function onResolve() {
        do_check_eq(order, 1);
        order++;
      }
    );

    let newPromise = promise.then(
      function onResolve() {
        do_check_eq(order, 2);
      }
    );

    deferred.resolve();

    
    return newPromise;
  }));


tests.push(
  make_promise_test(function then_returns_before_callbacks(test) {
    let promise = Promise.resolve();

    let order = 0;

    promise.then(
      function onResolve() {
        do_check_eq(order, 0);
        order++;
      }
    );

    promise.then(
      function onResolve() {
        do_check_eq(order, 1);
        order++;
      }
    );

    
    return promise.then(
      function onResolve() {
        do_check_eq(order, 2);
      }
    );
  }));



tests.push(make_promise_test(
  function notification_once(test) {
    
    const SIZE = 10;
    const RESULT = "this is an arbitrary value";

    
    let expected = SIZE;

    
    let notified = [];

    
    let observed = Promise.defer();
    let result = Promise.defer();

    let install_observer = function install_observer(i) {
      observe_failures(observed.promise.then(
        function onSuccess(value) {
          do_check_true(!notified[i], "Ensuring that observer is notified at most once");
          notified[i] = true;

          do_check_eq(value, RESULT, "Ensuring that the observed value is correct");
          if (--expected == 0) {
            result.resolve();
          }
        }));
    };

    
    let i;
    for (i = 0; i < SIZE/2; ++i) {
      install_observer(i);
    }

    observed.resolve(RESULT);

    
    for(;i < SIZE; ++i) {
      install_observer(i);
    }

    
    for (i = 0; i < 10; ++i) {
      observed.resolve(RESULT);
      observed.reject();
    }

    return result;
  }));




tests.push(
  make_promise_test(function exceptions_do_not_stop_notifications(test)  {
    let source = Promise.defer();

    let exception_thrown = false;
    let exception_content = new Error("Boom!");

    let observer_1 = source.promise.then(
      function onResolve() {
        exception_thrown = true;
        throw exception_content;
      });

    let observer_2 = source.promise.then(
      function onResolve() {
        do_check_true(exception_thrown, "Second observer called after first observer has thrown");
      }
    );

    let result = observer_1.then(
      function onResolve() {
        do_throw("observer_1 should not have resolved");
      },
      function onReject(reason) {
        do_check_true(reason == exception_content, "Obtained correct rejection");
      }
    );

    source.resolve();
    return result;
  }
));



tests.push(
  make_promise_test(function subsequent_resolves_are_ignored(test) {
    let deferred = Promise.defer();
    deferred.resolve(1);
    deferred.resolve(2);
    deferred.reject(3);

    let result = deferred.promise.then(
      function onResolve(value) {
        do_check_eq(value, 1, "Resolution chose the first value");
      },
      function onReject(reason) {
        do_throw("Obtained a rejection while the promise was already resolved");
      }
    );

    return result;
  }));



tests.push(
  make_promise_test(function subsequent_rejects_are_ignored(test) {
    let deferred = Promise.defer();
    deferred.reject(1);
    deferred.reject(2);
    deferred.resolve(3);

    let result = deferred.promise.then(
      function onResolve() {
        do_throw("Obtained a resolution while the promise was already rejected");
      },
      function onReject(reason) {
        do_check_eq(reason, 1, "Rejection chose the first value");
      }
    );

    return result;
  }));



tests.push(
  make_promise_test(function recovery(test) {
    let boom = new Error("Boom!");
    let deferred = Promise.defer();
    const RESULT = "An arbitrary value";

    let promise = deferred.promise.then(
      function onResolve() {
        do_throw("A rejected promise should not resolve");
      },
      function onReject(reason) {
        do_check_true(reason == boom, "Promise was rejected with the correct error");
        return RESULT;
      }
    );

    promise = promise.then(
      function onResolve(value) {
        do_check_eq(value, RESULT, "Promise was recovered with the correct value");
      }
    );

    deferred.reject(boom);
    return promise;
  }));




tests.push(
  make_promise_test(function recovery_with_promise(test) {
    let boom = new Error("Arbitrary error");
    let deferred = Promise.defer();
    const RESULT = "An arbitrary value";
    const boom2 = new Error("Another arbitrary error");

    
    let promise = deferred.promise.then(
      function onResolve() {
        do_throw("A rejected promise should not resolve");
      },
      function onReject(reason) {
        do_check_true(reason == boom, "Promise was rejected with the correct error");
        return Promise.resolve(RESULT);
      }
    );

    
    promise = promise.then(
      function onResolve(value) {
        do_check_eq(value, RESULT, "Promise was recovered with the correct value");
        return Promise.reject(boom2);
      }
    );

    promise = promise.then(
      null,
      function onReject(reason) {
        do_check_eq(reason, boom2, "Rejection was propagated with the correct " +
                "reason, through a promise");
      }
    );

    deferred.reject(boom);
    return promise;
  }));


tests.push(
  make_promise_test(function test_propagation(test) {
    const RESULT = "Yet another arbitrary value";
    let d1 = Promise.defer();
    let d2 = Promise.defer();
    let d3 = Promise.defer();

    d3.resolve(d2.promise);
    d2.resolve(d1.promise);
    d1.resolve(RESULT);

    return d3.promise.then(
      function onSuccess(value) {
        do_check_eq(value, RESULT, "Resolution with a promise eventually yielded "
                + " the correct result");
      }
    );
  }));


tests.push(
  make_promise_test(function test_chaining(test) {
    let error_1 = new Error("Error 1");
    let error_2 = new Error("Error 2");
    let result_1 = "First result";
    let result_2 = "Second result";
    let result_3 = "Third result";

    let source = Promise.defer();

    let promise = source.promise.then().then();

    source.resolve(result_1);

    
    promise = promise.then(
      function onSuccess(result) {
        do_check_eq(result, result_1, "Result was propagated correctly through " +
                " several applications of |then|");
        return result_2;
      }
    );

    
    promise = promise.then(
      null,
      function onReject() {
        do_throw("Incorrect rejection");
      }
    );

    
    promise = promise.then(
      function onResolve(value) {
        do_check_eq(value, result_2, "Result was propagated correctly once again");
      }
    );

    
    promise = promise.then(
      function onResolve() {
        throw error_1;
      }
    );

    promise = promise.then(
      function onResolve() {
        do_throw("Incorrect resolution: the exception should have caused a rejection");
      }
    );

    promise = promise.then(
      null,
      function onReject(reason) {
        do_check_true(reason == error_1, "Reason was propagated correctly");
        throw error_2;
      }
    );

    promise = promise.then(
      null,
      function onReject(reason) {
        do_check_true(reason == error_2, "Throwing an error altered the reason " +
            "as expected");
        return result_3;
      }
    );

    promise = promise.then(
      function onResolve(result) {
        do_check_eq(result, result_3, "Error was correctly recovered");
      }
    );

    return promise;
  }));


tests.push(
  make_promise_test(function resolve_to_rejected(test) {
    let source = Promise.defer();
    let error = new Error("Boom");

    let promise = source.promise.then(
      function onResolve() {
        do_throw("Incorrect call to onResolve listener");
      },
      function onReject(reason) {
        do_check_eq(reason, error, "Rejection lead to the expected reason");
      }
    );

    source.resolve(Promise.reject(error));

    return promise;
  }));


tests.push(
  make_promise_test(function test_resolve(test) {
    const RESULT = "arbitrary value";
    let promise = Promise.resolve(RESULT).then(
      function onResolve(result) {
        do_check_eq(result, RESULT, "Promise.resolve propagated the correct result");
      }
    );
    return promise;
  }));


tests.push(
  make_promise_test(function then_returns_before_callbacks(test) {
    let promise = Promise.resolve();

    let thenExecuted = false;

    promise = promise.then(
      function onResolve() {
        thenExecuted = true;
      }
    );

    do_check_false(thenExecuted);

    return promise;
  }));


tests.push(
  make_promise_test(function chaining_short_stack(test) {
    let source = Promise.defer();
    let promise = source.promise;

    const NUM_ITERATIONS = 100;

    for (let i = 0; i < NUM_ITERATIONS; i++) {
      promise = promise.then(
        function onResolve(result) {
          return result + ".";
        }
      );
    }

    promise = promise.then(
      function onResolve(result) {
        
        let expectedString = new Array(1 + NUM_ITERATIONS).join(".");
        do_check_true(result == expectedString);

        
        let stackFrameCount = 0;
        let stackFrame = Components.stack;
        while (stackFrame) {
          stackFrameCount++;
          stackFrame = stackFrame.caller;
        }

        do_check_true(stackFrameCount < NUM_ITERATIONS);
      }
    );

    source.resolve("");

    return promise;
  }));



tests.push(
  make_promise_test(function all_resolve(test) {
    let d1 = Promise.defer();
    let d2 = Promise.defer();
    let d3 = Promise.defer();

    d3.resolve(4);
    d2.resolve(2);
    do_execute_soon(() => d1.resolve(1));

    let promises = [d1.promise, d2.promise, 3, d3.promise];

    return Promise.all(promises).then(
      function onResolve([val1, val2, val3, val4]) {
        do_check_eq(val1, 1);
        do_check_eq(val2, 2);
        do_check_eq(val3, 3);
        do_check_eq(val4, 4);
      }
    );
  }));



tests.push(
  make_promise_test(function all_reject(test) {
    let error = new Error("Boom");

    let d1 = Promise.defer();
    let d2 = Promise.defer();
    let d3 = Promise.defer();

    d3.resolve(3);
    d2.resolve(2);
    do_execute_soon(() => d1.reject(error));

    let promises = [d1.promise, d2.promise, d3.promise];

    return Promise.all(promises).then(
      function onResolve() {
        do_throw("Incorrect call to onResolve listener");
      },
      function onReject(reason) {
        do_check_eq(reason, error, "Rejection lead to the expected reason");
      }
    );
  }));



tests.push(
  make_promise_test(function all_resolve_no_promises(test) {
    try {
      Promise.all(null);
      do_check_true(false, "all() should only accept arrays.");
    } catch (e) {
      do_check_true(true, "all() fails when first the arg is not an array.");
    }

    let p1 = Promise.all([]).then(
      function onResolve(val) {
        do_check_true(Array.isArray(val) && val.length == 0);
      }
    );

    let p2 = Promise.all([1, 2, 3]).then(
      function onResolve([val1, val2, val3]) {
        do_check_eq(val1, 1);
        do_check_eq(val2, 2);
        do_check_eq(val3, 3);
      }
    );

    return Promise.all([p1, p2]);
  }));

function run_test()
{
  do_test_pending();
  run_promise_tests(tests, do_test_finished);
}
