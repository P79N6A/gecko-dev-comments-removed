


'use strict';

const {PushDB, PushService} = serviceExports;

function run_test() {
  do_get_profile();
  setPrefs({
    userAgentID: '6faed1f0-1439-4aac-a978-db21c81cd5eb'
  });
  run_next_test();
}

add_task(function* test_registrations_error() {
  let db = new PushDB();
  do_register_cleanup(() => cleanupDatabase(db));

  PushService.init({
    networkInfo: new MockDesktopNetworkInfo(),
    db: makeStub(db, {
      getByScope(prev, scope, successCb, failureCb) {
        failureCb('oops');
      }
    }),
    makeWebSocket(uri) {
      return new MockWebSocket(uri);
    }
  });

  yield rejects(
    PushNotificationService.registration('https://example.net/1'),
    function(error) {
      return error == 'Database error';
    },
    'Wrong message'
  );
});
