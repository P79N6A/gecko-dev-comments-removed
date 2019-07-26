




Components.utils.import("resource://gre/modules/OrderedBroadcast.jsm");
Components.utils.import("resource://gre/modules/commonjs/sdk/core/promise.js");

let _observerId = 0;

function makeObserver() {
  let deferred = Promise.defer();

  let ret = {
    id: _observerId++,
    count: 0,
    promise: deferred.promise,
    callback: function(data, token, action) {
      ret.count += 1;
      deferred.resolve({ data: data,
                         token: token,
                         action: action });
    },
  };

  return ret;
};

add_task(function test_send() {
  let deferred = Promise.defer();

  let observer = makeObserver();

  sendOrderedBroadcast("org.mozilla.gecko.test.receiver",
                       { a: "bcde", b: 1234 }, observer.callback);

  let value = yield observer.promise;

  do_check_eq(observer.count, 1);

  
  do_check_neq(value, null);
  do_check_neq(value.token, null);
  do_check_eq(value.token.a, "bcde");
  do_check_eq(value.token.b, 1234);
  do_check_eq(value.action, "org.mozilla.gecko.test.receiver");

  
  do_check_neq(value.data, null);
  do_check_eq(value.data.c, "efg");
  do_check_eq(value.data.d, 456);
});

add_task(function test_null_token() {
  let deferred = Promise.defer();

  let observer = makeObserver();

  sendOrderedBroadcast("org.mozilla.gecko.test.receiver",
                       null, observer.callback);

  let value = yield observer.promise;

  do_check_eq(observer.count, 1);

  
  do_check_neq(value, null);
  do_check_eq(value.token, null);
  do_check_eq(value.action, "org.mozilla.gecko.test.receiver");

  
  do_check_neq(value.data, null);
  do_check_eq(value.data.c, "efg");
  do_check_eq(value.data.d, 456);
});

add_task(function test_permission() {
  let deferred = Promise.defer();

  let observer = makeObserver();

  sendOrderedBroadcast("org.mozilla.gecko.test.receiver",
                       null, observer.callback,
                       "org.mozilla.gecko.fake.permission");

  let value = yield observer.promise;

  do_check_eq(observer.count, 1);

  
  do_check_neq(value, null);
  do_check_eq(value.token, null);
  do_check_eq(value.action, "org.mozilla.gecko.test.receiver");

  
  
  do_check_eq(value.data, null);
});

add_task(function test_send_no_receiver() {
  let deferred = Promise.defer();

  let observer = makeObserver();

  sendOrderedBroadcast("org.mozilla.gecko.test.no.receiver",
                       { a: "bcd", b: 123 }, observer.callback);

  let value = yield observer.promise;

  do_check_eq(observer.count, 1);

  
  
  do_check_neq(value, null);
  do_check_neq(value.token, null);
  do_check_eq(value.token.a, "bcd");
  do_check_eq(value.token.b, 123);
  do_check_eq(value.action, "org.mozilla.gecko.test.no.receiver");
  do_check_eq(value.data, null);
});

run_next_test();
