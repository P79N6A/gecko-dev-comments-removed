


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'mmdb_head.js';

const DBNAME = "test_mmdb_full_storage:" + newUUID();

let gIsDiskFull = true;

function newSavableMessage() {
  return {
    type: "sms",
    sender: "+0987654321",
    receiver: "+1234567890",
    body: "quick fox jump over the lazy dog",
    deliveryStatusRequested: false,
    messageClass: "normal",
    timestamp: Date.now(),
    iccId: "1029384756"
  };
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

  gIsDiskFull = true;
  return saveSendingMessage(aMmdb, newSavableMessage())
    
    
    .then((aValue) => aValue[0],
          (aValue) => aValue[0])
    .then(isFileNoDeviceSpaceError);
}

function testSaveReceivedMessage(aMmdb) {
  log("testSaveReceivedMessage()");

  gIsDiskFull = true;
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    
    .then((aValue) => aValue[0],
          (aValue) => aValue[0])
    .then(isFileNoDeviceSpaceError);
}

function testGetMessageRecordById(aMmdb) {
  log("testGetMessageRecordById()");

  gIsDiskFull = false;
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    .then(function(aValue) {
      let domMessage = aValue[1];

      gIsDiskFull = true;
      return getMessageRecordById(aMmdb, domMessage.id);
    });
}

function testMarkMessageRead(aMmdb) {
  log("testMarkMessageRead()");

  gIsDiskFull = false;
  return saveReceivedMessage(aMmdb, newSavableMessage())
    
    .then(function(aValue) {
      let domMessage = aValue[1];

      gIsDiskFull = true;
      return markMessageRead(aMmdb, domMessage.id, true)
        .then(null, (aValue) => aValue)
        .then(isCallbackStorageFullError);
    });
}

function testCreateMessageCursor(aMmdb) {
  log("testCreateMessageCursor()");

  gIsDiskFull = true;
  return createMessageCursor(aMmdb, {}, false);
}

function testCreateThreadCursor(aMmdb) {
  log("testCreateThreadCursor()");

  gIsDiskFull = true;
  return createThreadCursor(aMmdb);
}

startTestBase(function testCaseMain() {

  let mmdb = newMobileMessageDB();
  return initMobileMessageDB(mmdb, DBNAME, 0)
    .then(function() {
      mmdb.isDiskFull = function() {
        return gIsDiskFull;
      };
    })

    .then(() => testSaveSendingMessage(mmdb))
    .then(() => testSaveReceivedMessage(mmdb))
    .then(() => testGetMessageRecordById(mmdb))
    .then(() => testMarkMessageRead(mmdb))
    .then(() => testCreateMessageCursor(mmdb))
    .then(() => testCreateThreadCursor(mmdb))

    .then(() => closeMobileMessageDB(mmdb));
});
