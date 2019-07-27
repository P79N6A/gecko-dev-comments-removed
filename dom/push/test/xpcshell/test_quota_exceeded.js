


'use strict';

const {PushDB, PushService, PushServiceWebSocket} = serviceExports;

Cu.import("resource://gre/modules/Task.jsm");

const userAgentID = '7eb873f9-8d47-4218-804b-fff78dc04e88';

function run_test() {
  do_get_profile();
  setPrefs({
    userAgentID,
  });
  run_next_test();
}

add_task(function* test_expiration_origin_threshold() {
  let db = PushServiceWebSocket.newPushDB();
  do_register_cleanup(() => db.drop().then(_ => db.close()));

  yield db.put({
    channelID: 'eb33fc90-c883-4267-b5cb-613969e8e349',
    pushEndpoint: 'https://example.org/push/1',
    scope: 'https://example.com/auctions',
    pushCount: 0,
    lastPush: 0,
    version: null,
    originAttributes: '',
    quota: 16,
  });
  yield db.put({
    channelID: '46cc6f6a-c106-4ffa-bb7c-55c60bd50c41',
    pushEndpoint: 'https://example.org/push/2',
    scope: 'https://example.com/deals',
    pushCount: 0,
    lastPush: 0,
    version: null,
    originAttributes: '',
    quota: 16,
  });

  
  
  yield addVisit({
    uri: 'https://example.com/login',
    title: 'Sign in to see your auctions',
    visits: [{
      visitDate: (Date.now() - 7 * 24 * 60 * 60 * 1000) * 1000,
      transitionType: Ci.nsINavHistoryService.TRANSITION_LINK,
    }],
  });

  
  yield addVisit({
    uri: 'https://example.com/auctions',
    title: 'Your auctions',
    visits: [{
      visitDate: (Date.now() - 2 * 24 * 60 * 60 * 1000) * 1000,
      transitionType: Ci.nsINavHistoryService.TRANSITION_LINK,
    }],
  });

  
  yield addVisit({
    uri: 'https://example.com/invoices/invoice.pdf',
    title: 'Invoice #123',
    visits: [{
      visitDate: (Date.now() - 1 * 24 * 60 * 60 * 1000) * 1000,
      transitionType: Ci.nsINavHistoryService.TRANSITION_EMBED,
    }, {
      visitDate: Date.now() * 1000,
      transitionType: Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
    }],
  });

  
  
  
  
  let updates = 0;
  let notifyPromise = promiseObserverNotification('push-notification', (subject, data) => {
    updates++;
    return updates == 6;
  });
  let unregisterDefer = Promise.defer();

  PushService.init({
    serverURI: 'wss://push.example.org/',
    networkInfo: new MockDesktopNetworkInfo(),
    db,
    makeWebSocket(uri) {
      return new MockWebSocket(uri, {
        onHello(request) {
          deepEqual(request.channelIDs.sort(), [
            '46cc6f6a-c106-4ffa-bb7c-55c60bd50c41',
            'eb33fc90-c883-4267-b5cb-613969e8e349',
          ], 'Wrong active registrations in handshake');
          this.serverSendMsg(JSON.stringify({
            messageType: 'hello',
            status: 200,
            uaid: userAgentID,
          }));
          
          
          
          for (let version = 1; version <= 6; version++) {
            this.serverSendMsg(JSON.stringify({
              messageType: 'notification',
              updates: [{
                channelID: 'eb33fc90-c883-4267-b5cb-613969e8e349',
                version,
              }],
            }));
          }
          
          
          this.serverSendMsg(JSON.stringify({
            messageType: 'notification',
            updates: [{
              channelID: '46cc6f6a-c106-4ffa-bb7c-55c60bd50c41',
              version: 1,
            }],
          }));
        },
        onUnregister(request) {
          equal(request.channelID, 'eb33fc90-c883-4267-b5cb-613969e8e349', 'Unregistered wrong channel ID');
          unregisterDefer.resolve();
        },
        
        
        onACK(request) {},
      });
    },
  });

  yield waitForPromise(unregisterDefer.promise, DEFAULT_TIMEOUT,
    'Timed out waiting for unregister request');

  yield waitForPromise(notifyPromise, DEFAULT_TIMEOUT,
    'Timed out waiting for notifications');

  let expiredRecord = yield db.getByKeyID('eb33fc90-c883-4267-b5cb-613969e8e349');
  strictEqual(expiredRecord.quota, 0, 'Expired record not updated');
});
