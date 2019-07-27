


'use strict';

const {PushDB, PushService} = serviceExports;

function run_test() {
  do_get_profile();
  setPrefs();
  disableServiceWorkerEvents(
    'https://example.com/a',
    'https://example.com/b',
    'https://example.com/c'
  );
  run_next_test();
}

add_task(function* test_notification_error() {
  let db = new PushDB();
  let promiseDB = promisifyDatabase(db);
  do_register_cleanup(() => cleanupDatabase(db));
  let records = [{
    channelID: 'f04f1e46-9139-4826-b2d1-9411b0821283',
    pushEndpoint: 'https://example.org/update/success-1',
    scope: 'https://example.com/a',
    version: 1
  }, {
    channelID: '3c3930ba-44de-40dc-a7ca-8a133ec1a866',
    pushEndpoint: 'https://example.org/update/error',
    scope: 'https://example.com/b',
    version: 2
  }, {
    channelID: 'b63f7bef-0a0d-4236-b41e-086a69dfd316',
    pushEndpoint: 'https://example.org/update/success-2',
    scope: 'https://example.com/c',
    version: 3
  }];
  for (let record of records) {
    yield promiseDB.put(record);
  }

  let notifyPromise = Promise.all([
    promiseObserverNotification(
      'push-notification',
      (subject, data) => data == 'https://example.com/a'
    ),
    promiseObserverNotification(
      'push-notification',
      (subject, data) => data == 'https://example.com/c'
    )
  ]);

  let ackDefer = Promise.defer();
  let ackDone = after(records.length, ackDefer.resolve);
  PushService.init({
    networkInfo: new MockDesktopNetworkInfo(),
    db: makeStub(db, {
      getByChannelID(prev, channelID, successCb, failureCb) {
        if (channelID == '3c3930ba-44de-40dc-a7ca-8a133ec1a866') {
          return failureCb('splines not reticulated');
        }
        return prev.call(this, channelID, successCb, failureCb);
      }
    }),
    makeWebSocket(uri) {
      return new MockWebSocket(uri, {
        onHello(request) {
          deepEqual(request.channelIDs.sort(), [
            '3c3930ba-44de-40dc-a7ca-8a133ec1a866',
            'b63f7bef-0a0d-4236-b41e-086a69dfd316',
            'f04f1e46-9139-4826-b2d1-9411b0821283'
          ], 'Wrong channel list');
          this.serverSendMsg(JSON.stringify({
            messageType: 'hello',
            status: 200,
            uaid: '3c7462fc-270f-45be-a459-b9d631b0d093'
          }));
          this.serverSendMsg(JSON.stringify({
            messageType: 'notification',
            updates: records.map(({channelID, version}) =>
              ({channelID, version: ++version}))
          }));
        },
        
        
        onACK: ackDone
      });
    }
  });

  let [a, c] = yield waitForPromise(
    notifyPromise,
    3000,
    'Timed out waiting for notifications'
  );
  let aPush = a.subject.QueryInterface(Ci.nsIPushObserverNotification);
  equal(aPush.pushEndpoint, 'https://example.org/update/success-1',
    'Wrong endpoint for notification A');
  equal(aPush.version, 2, 'Wrong version for notification A');

  let cPush = c.subject.QueryInterface(Ci.nsIPushObserverNotification);
  equal(cPush.pushEndpoint, 'https://example.org/update/success-2',
    'Wrong endpoint for notification C');
  equal(cPush.version, 4, 'Wrong version for notification C');

  yield waitForPromise(ackDefer.promise, 3000,
    'Timed out waiting for acknowledgements');

  let aRecord = yield promiseDB.getByScope('https://example.com/a');
  equal(aRecord.channelID, 'f04f1e46-9139-4826-b2d1-9411b0821283',
    'Wrong channel ID for record A');
  strictEqual(aRecord.version, 2,
    'Should return the new version for record A');

  let bRecord = yield promiseDB.getByScope('https://example.com/b');
  equal(bRecord.channelID, '3c3930ba-44de-40dc-a7ca-8a133ec1a866',
    'Wrong channel ID for record B');
  strictEqual(bRecord.version, 2,
    'Should return the previous version for record B');

  let cRecord = yield promiseDB.getByScope('https://example.com/c');
  equal(cRecord.channelID, 'b63f7bef-0a0d-4236-b41e-086a69dfd316',
    'Wrong channel ID for record C');
  strictEqual(cRecord.version, 4,
    'Should return the new version for record C');
});
