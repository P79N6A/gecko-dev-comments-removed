


MARIONETTE_CONTEXT = "chrome";

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;





let MMDB;






function newMobileMessageDB() {
  if (!MMDB) {
    MMDB = Cu.import("resource://gre/modules/MobileMessageDB.jsm", {});
    is(typeof MMDB.MobileMessageDB, "function", "MMDB.MobileMessageDB");
  }

  let mmdb = new MMDB.MobileMessageDB();
  ok(mmdb, "MobileMessageDB instance");
  return mmdb;
}


















function initMobileMessageDB(aMmdb, aDbName, aDbVersion) {
  let deferred = Promise.defer();

  aMmdb.init(aDbName, aDbVersion, function(aError) {
    if (aError) {
      deferred.reject(aMmdb);
    } else {
      deferred.resolve(aMmdb);
    }
  });

  return deferred.promise;
}









function closeMobileMessageDB(aMmdb) {
  aMmdb.close();
  return aMmdb;
}
























function callMmdbMethod(aMmdb, aMethodName) {
  let deferred = Promise.defer();

  let args = Array.slice(arguments, 2);
  args.push({
    notify: function(aRv) {
      if (!Components.isSuccessCode(aRv)) {
        ok(true, aMethodName + " returns a unsuccessful code: " + aRv);
        deferred.reject(Array.slice(arguments));
      } else {
        ok(true, aMethodName + " returns a successful code: " + aRv);
        deferred.resolve(Array.slice(arguments));
      }
    }
  });
  aMmdb[aMethodName].apply(aMmdb, args);

  return deferred.promise;
}









function saveSendingMessage(aMmdb, aMessage) {
  return callMmdbMethod(aMmdb, "saveSendingMessage", aMessage);
}









function saveReceivedMessage(aMmdb, aMessage) {
  return callMmdbMethod(aMmdb, "saveReceivedMessage", aMessage);
}









function setMessageDeliveryByMessageId(aMmdb, aMessageId, aReceiver, aDelivery,
                                       aDeliveryStatus, aEnvelopeId) {
  return callMmdbMethod(aMmdb, "setMessageDeliveryByMessageId", aMessageId,
                        aReceiver, aDelivery, aDeliveryStatus, aEnvelopeId);
}










function setMessageDeliveryStatusByEnvelopeId(aMmdb, aEnvelopeId, aReceiver,
                                              aDeliveryStatus) {
  return callMmdbMethod(aMmdb, "setMessageDeliveryStatusByEnvelopeId",
                        aMmdb, aEnvelopeId, aReceiver, aDeliveryStatus);
}










function setMessageReadStatusByEnvelopeId(aMmdb, aEnvelopeId, aReceiver,
                                          aReadStatus) {
  return callMmdbMethod(aMmdb, "setMessageReadStatusByEnvelopeId",
                        aEnvelopeId, aReceiver, aReadStatus);
}










function getMessageRecordByTransactionId(aMmdb, aTransactionId) {
  return callMmdbMethod(aMmdb, "getMessageRecordByTransactionId",
                        aTransactionId);
}









function getMessageRecordById(aMmdb, aMessageId) {
  return callMmdbMethod(aMmdb, "getMessageRecordById", aMessageId);
}









function markMessageRead(aMmdb, aMessageId, aRead) {
  let deferred = Promise.defer();

  aMmdb.markMessageRead(aMessageId, aRead, false, {
    notifyMarkMessageReadFailed: function(aRv) {
      ok(true, "markMessageRead returns a unsuccessful code: " + aRv);
      deferred.reject(aRv);
    },

    notifyMessageMarkedRead: function(aRead) {
      ok(true, "markMessageRead returns a successful code: " + Cr.NS_OK);
      deferred.resolve(Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR);
    }
  });

  return deferred.promise;
}









function deleteMessage(aMmdb, aMessageIds, aLength) {
  let deferred = Promise.defer();

  aMmdb.deleteMessage(aMessageIds, aLength, {
    notifyDeleteMessageFailed: function(aRv) {
      ok(true, "deleteMessage returns a unsuccessful code: " + aRv);
      deferred.reject(aRv);
    },

    notifyMessageDeleted: function(aDeleted, aLength) {
      ok(true, "deleteMessage successfully!");
      deferred.resolve(aDeleted);
    }
  });

  return deferred.promise;
}









function saveSmsSegment(aMmdb, aSmsSegment) {
  return callMmdbMethod(aMmdb, "saveSmsSegment", aSmsSegment);
}




















function createMmdbCursor(aMmdb, aMethodName) {
  let deferred = Promise.defer();

  let cursor;
  let results = [];
  let args = Array.slice(arguments, 2);
  args.push({
    notifyCursorError: function(aRv) {
      ok(true, "notifyCursorError: " + aRv);
      deferred.reject([aRv, results]);
    },

    notifyCursorResult: function(aResults, aSize) {
      ok(true, "notifyCursorResult: " + aResults.map(function(aElement) { return aElement.id; }));
      results = results.concat(aResults);
      cursor.handleContinue();
    },

    notifyCursorDone: function() {
      ok(true, "notifyCursorDone");
      deferred.resolve([Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR, results]);
    }
  });

  cursor = aMmdb[aMethodName].apply(aMmdb, args);

  return deferred.promise;
}









function createMessageCursor(aMmdb, aStartDate = null, aEndDate = null,
                             aNumbers = null, aDelivery = null, aRead = null,
                             aThreadId = null, aReverse = false) {
  return createMmdbCursor(aMmdb, "createMessageCursor",
                          aStartDate !== null,
                          aStartDate || 0,
                          aEndDate !== null,
                          aEndDate || 0,
                          aNumbers || null,
                          aNumbers && aNumbers.length || 0,
                          aDelivery || null,
                          aRead !== null,
                          aRead || false,
                          aThreadId || 0,
                          aReverse || false);
}









function createThreadCursor(aMmdb) {
  return createMmdbCursor(aMmdb, "createThreadCursor");
}


let _uuidGenerator;






function newUUID() {
  if (!_uuidGenerator) {
    _uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                     .getService(Ci.nsIUUIDGenerator);
    ok(_uuidGenerator, "uuidGenerator");
  }

  return _uuidGenerator.generateUUID().toString();
}




function cleanUp() {
  
  ok(true, "permissions flushed");

  finish();
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
    .then(aTestCaseMain)
    .then(null, function() {
      ok(false, 'promise rejects during test.');
    })
    .then(cleanUp);
}
