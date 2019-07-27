


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'mmdb_head.js';

const DBNAME = "test_mmdb_full_storage:" + newUUID();

function newSavableMessage(aSender, aReceiver) {
  return {
    type: "sms",
    sender: aSender ? aSender : "+0987654321",
    receiver: aReceiver? aReceiver : "+1234567890",
    body: "quick fox jump over the lazy dog",
    deliveryStatusRequested: false,
    messageClass: "normal",
    timestamp: Date.now(),
    iccId: "1029384756"
  };
}

function setStorageFull(aFull) {
  SpecialPowers.notifyObserversInParentProcess(null,
                                               "disk-space-watcher",
                                               aFull ? "full" : "free");
}

function isFileNoDeviceSpaceError(aErrorResult) {
  is(aErrorResult, Cr.NS_ERROR_FILE_NO_DEVICE_SPACE, "Database error code");
}

function isCallbackStorageFullError(aErrorCode) {
  is(aErrorCode, Ci.nsIMobileMessageCallback.STORAGE_FULL_ERROR,
     "nsIMobileMessageCallback error code");
}

function testSaveSendingMessage(aMmdb) {
  log("testSaveSendingMessage()");

  setStorageFull(true);
  return saveSendingMessage(aMmdb, newSavableMessage())
    
    
    .then((aValue) => aValue[0],
          (aValue) => aValue[0])
    .then(isFileNoDeviceSpaceError)
    .then(() => setStorageFull(false));
}

function testSaveReceivedMessage(aMmdb) {
  log("testSaveReceivedMessage()");

  setStorageFull(true);
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    
    .then((aValue) => aValue[0],
          (aValue) => aValue[0])
    .then(isFileNoDeviceSpaceError)
    .then(() => setStorageFull(false));
}

function testGetMessageRecordById(aMmdb) {
  log("testGetMessageRecordById()");

  setStorageFull(false);
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    .then(function(aValue) {
      let domMessage = aValue[1];

      setStorageFull(true);
      return getMessageRecordById(aMmdb, domMessage.id)
        .then(() => setStorageFull(false));
    });
}

function testMarkMessageRead(aMmdb) {
  log("testMarkMessageRead()");

  setStorageFull(false);
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    .then(function(aValue) {
      let domMessage = aValue[1];

      setStorageFull(true);
      return markMessageRead(aMmdb, domMessage.id, true)
        .then(null, (aValue) => aValue)
        .then(isCallbackStorageFullError)
        .then(() => setStorageFull(false));
    });
}

function testDeleteMessage(aMmdb) {
  log("testDeleteMessage()");

  
  
  
  
  
  
  
  
  
  
  

  let testAddress = "1111111111";
  let savedMsgIds = [];
  let promises = [];
  let numOfTestMessages = 5;

  setStorageFull(false);
  
  for (let i = 0; i < numOfTestMessages; i++) {
    promises.push(saveReceivedMessage(aMmdb, newSavableMessage(testAddress))
      .then((aValue) => { savedMsgIds.push(aValue[1].id); }));
  }

  return Promise.all(promises)
    .then(() => setStorageFull(true))

    
    .then(() => deleteMessage(aMmdb, [savedMsgIds[numOfTestMessages - 1]], 1))
    .then(null, (aValue) => aValue)
    .then(isCallbackStorageFullError)

    
    .then(() => deleteMessage(aMmdb, [savedMsgIds[0]], 1))
    .then(null, (aValue) => aValue)
    .then(isCallbackStorageFullError)

    
    .then(() => deleteMessage(aMmdb, savedMsgIds, savedMsgIds.length))

    .then(() => setStorageFull(false));
}

function testSaveSmsSegment(aMmdb) {
  log("testSaveSmsSegment()");

  let smsSegment = {
    sender: "+0987654321",
    encoding: 0x00, 
    iccId: "1029384756",
    segmentRef: 0,
    segmentSeq: 1,
    segmentMaxSeq: 3,
    body: "quick fox jump over the lazy dog"
  }

  setStorageFull(true);
  return saveSmsSegment(aMmdb, smsSegment)
    
    
    .then((aValue) => aValue[0],
          (aValue) => aValue[0])
    .then(isFileNoDeviceSpaceError)
    .then(() => setStorageFull(false));
}

function testCreateMessageCursor(aMmdb) {
  log("testCreateMessageCursor()");

  setStorageFull(true);
  return createMessageCursor(aMmdb)
    .then(() => setStorageFull(false));
}

function testCreateThreadCursor(aMmdb) {
  log("testCreateThreadCursor()");

  setStorageFull(true);
  return createThreadCursor(aMmdb)
    .then(() => setStorageFull(false));
}

startTestBase(function testCaseMain() {

  let mmdb = newMobileMessageDB();
  return initMobileMessageDB(mmdb, DBNAME, 0)

    .then(() => testSaveSendingMessage(mmdb))
    .then(() => testSaveReceivedMessage(mmdb))
    .then(() => testGetMessageRecordById(mmdb))
    .then(() => testMarkMessageRead(mmdb))
    .then(() => testDeleteMessage(mmdb))
    .then(() => testSaveSmsSegment(mmdb))
    .then(() => testCreateMessageCursor(mmdb))
    .then(() => testCreateThreadCursor(mmdb))

    .then(() => closeMobileMessageDB(mmdb));
});
