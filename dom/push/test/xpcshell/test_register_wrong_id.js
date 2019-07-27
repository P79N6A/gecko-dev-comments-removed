


'use strict';

const {PushDB, PushService} = serviceExports;

const userAgentID = '84afc774-6995-40d1-9c90-8c34ddcd0cb4';
const clientChannelID = '4b42a681c99e4dfbbb166a7e01a09b8b';
const serverChannelID = '3f5aeb89c6e8405a9569619522783436';

function run_test() {
  do_get_profile();
  setPrefs({
    userAgentID,
    requestTimeout: 1000,
    retryBaseInterval: 150
  });
  disableServiceWorkerEvents(
    'https://example.com/mismatched'
  );
  run_next_test();
}

add_task(function* test_register_wrong_id() {
  
  let registers = 0;
  let helloDefer = Promise.defer();
  let helloDone = after(2, helloDefer.resolve);

  PushService._generateID = () => clientChannelID;
  PushService.init({
    networkInfo: new MockDesktopNetworkInfo(),
    makeWebSocket(uri) {
      return new MockWebSocket(uri, {
        onHello(request) {
          this.serverSendMsg(JSON.stringify({
            messageType: 'hello',
            status: 200,
            uaid: userAgentID
          }));
          helloDone();
        },
        onRegister(request) {
          equal(request.channelID, clientChannelID, 'Register: wrong channel ID');
          registers++;
          this.serverSendMsg(JSON.stringify({
            messageType: 'register',
            status: 200,
            
            
            channelID: serverChannelID
          }));
        }
      });
    }
  });

  yield rejects(
    PushNotificationService.register('https://example.com/mismatched'),
    function(error) {
      return error == 'TimeoutError';
    },
    'Wrong error for mismatched register reply'
  );

  yield waitForPromise(helloDefer.promise, 3000,
    'Reconnect after mismatched register reply timed out');
  equal(registers, 1, 'Wrong register count');
});
