


'use strict';

const {PushDB, PushService} = serviceExports;

function run_test() {
  do_get_profile();
  setPrefs({
    requestTimeout: 1000,
    retryBaseInterval: 150
  });
  disableServiceWorkerEvents(
    'https://example.com/page/1'
  );
  run_next_test();
}

add_task(function* test_register_request_queue() {
  let db = new PushDB();
  let promiseDB = promisifyDatabase(db);
  do_register_cleanup(() => cleanupDatabase(db));

  let helloDefer = Promise.defer();
  let onHello = after(2, function onHello(request) {
    this.serverSendMsg(JSON.stringify({
      messageType: 'hello',
      status: 200,
      uaid: '54b08a9e-59c6-4ed7-bb54-f4fd60d6f606'
    }));
    helloDefer.resolve();
  });
  PushService.init({
    networkInfo: new MockDesktopNetworkInfo(),
    db,
    makeWebSocket(uri) {
      return new MockWebSocket(uri, {
        onHello,
        onRegister() {
          ok(false, 'Should cancel timed-out requests');
        }
      });
    }
  });

  let firstRegister = PushNotificationService.register(
    'https://example.com/page/1'
  );
  let secondRegister = PushNotificationService.register(
    'https://example.com/page/1'
  );

  yield waitForPromise(Promise.all([
    rejects(firstRegister, function(error) {
      return error == 'TimeoutError';
    }, 'Should time out the first request'),
    rejects(secondRegister, function(error) {
      return error == 'TimeoutError';
    }, 'Should time out the second request')
  ]), DEFAULT_TIMEOUT, 'Queued requests did not time out');

  yield waitForPromise(helloDefer.promise, DEFAULT_TIMEOUT,
    'Timed out waiting for reconnect');
});
