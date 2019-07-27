


'use strict';

const {PushDB, PushService} = serviceExports;

const userAgentID = 'a722e448-c481-4c48-aea0-fc411cb7c9ed';

function run_test() {
  do_get_profile();
  setPrefs({userAgentID});
  run_next_test();
}


add_task(function* test_registration_none() {
  PushService.init({
    networkInfo: new MockDesktopNetworkInfo(),
    makeWebSocket(uri) {
      return new MockWebSocket(uri);
    }
  });

  let registration = yield PushNotificationService.registration(
    'https://example.net/1');
  ok(!registration, 'Should not open a connection without registration');
});
