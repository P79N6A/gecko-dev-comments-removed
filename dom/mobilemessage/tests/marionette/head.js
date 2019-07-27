


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;














function pushPrefEnv(aPrefs) {
  let deferred = Promise.defer();

  SpecialPowers.pushPrefEnv(aPrefs, function() {
    ok(true, "preferences pushed: " + JSON.stringify(aPrefs));
    deferred.resolve();
  });

  return deferred.promise;
}












let manager;
function ensureMobileMessage() {
  let deferred = Promise.defer();

  let permissions = [{
    "type": "sms",
    "allow": 1,
    "context": document,
  }];
  SpecialPowers.pushPermissions(permissions, function() {
    ok(true, "permissions pushed: " + JSON.stringify(permissions));

    manager = window.navigator.mozMobileMessage;
    if (manager) {
      log("navigator.mozMobileMessage is instance of " + manager.constructor);
    } else {
      log("navigator.mozMobileMessage is undefined.");
    }

    if (manager instanceof MozMobileMessageManager) {
      deferred.resolve(manager);
    } else {
      deferred.reject();
    }
  });

  return deferred.promise;
}
















function waitForManagerEvent(aEventName, aMatchFunc) {
  let deferred = Promise.defer();

  manager.addEventListener(aEventName, function onevent(aEvent) {
    if (aMatchFunc && !aMatchFunc(aEvent)) {
      ok(true, "MobileMessageManager event '" + aEventName + "' got" +
               " but is not interested.");
      return;
    }

    ok(true, "MobileMessageManager event '" + aEventName + "' got.");
    manager.removeEventListener(aEventName, onevent);
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}












function wrapDomRequestAsPromise(aRequest) {
  let deferred = Promise.defer();

  ok(aRequest instanceof DOMRequest,
     "aRequest is instanceof " + aRequest.constructor);

  aRequest.addEventListener("success", function(aEvent) {
    deferred.resolve(aEvent);
  });
  aRequest.addEventListener("error", function(aEvent) {
    deferred.reject(aEvent);
  });

  return deferred.promise;
}
















function sendSmsWithSuccess(aReceiver, aText) {
  let request = manager.send(aReceiver, aText);
  return wrapDomRequestAsPromise(request)
    .then((aEvent) => { return aEvent.target.result; },
          (aEvent) => { throw aEvent.target.error; });
}


















function sendSmsWithFailure(aReceiver, aText) {
  let promises = [];
  promises.push(waitForManagerEvent("failed")
    .then((aEvent) => { return aEvent.message; }));

  let request = manager.send(aReceiver, aText);
  promises.push(wrapDomRequestAsPromise(request)
    .then((aEvent) => { throw aEvent; },
          (aEvent) => { return aEvent.target.error; }));

  return Promise.all(promises)
    .then((aResults) => { return { message: aResults[0],
                                   error: aResults[1] }; });
}



















function sendMmsWithFailure(aMmsParameters, aSendParameters) {
  let promises = [];
  promises.push(waitForManagerEvent("failed")
    .then((aEvent) => { return aEvent.message; }));

  let request = manager.sendMMS(aMmsParameters, aSendParameters);
  promises.push(wrapDomRequestAsPromise(request)
    .then((aEvent) => { throw aEvent; },
          (aEvent) => { return aEvent.target.error; }));

  return Promise.all(promises)
    .then((aResults) => { return { message: aResults[0],
                                   error: aResults[1] }; });
}













function getMessage(aId) {
  let request = manager.getMessage(aId);
  return wrapDomRequestAsPromise(request)
    .then((aEvent) => { return aEvent.target.result; });
}
















function getMessages(aFilter, aReverse) {
  let deferred = Promise.defer();

  if (!aFilter) {
    aFilter = new MozSmsFilter;
  }
  let messages = [];
  let cursor = manager.getMessages(aFilter, aReverse || false);
  cursor.onsuccess = function(aEvent) {
    if (cursor.result) {
      messages.push(cursor.result);
      cursor.continue();
      return;
    }

    deferred.resolve(messages);
  };
  cursor.onerror = deferred.reject.bind(deferred);

  return deferred.promise;
}












function getAllMessages() {
  return getMessages(null, false);
}












function getAllThreads() {
  let deferred = Promise.defer();

  let threads = [];
  let cursor = manager.getThreads();
  cursor.onsuccess = function(aEvent) {
    if (cursor.result) {
      threads.push(cursor.result);
      cursor.continue();
      return;
    }

    deferred.resolve(threads);
  };
  cursor.onerror = deferred.reject.bind(deferred);

  return deferred.promise;
}















function getThreadById(aThreadId) {
  return getAllThreads()
    .then(function(aThreads) {
      for (let thread of aThreads) {
        if (thread.id === aThreadId) {
          return thread;
        }
      }
      throw undefined;
    });
}















function deleteMessagesById(aMessageIds) {
  if (!aMessageIds.length) {
    ok(true, "no message to be deleted");
    return [];
  }

  let promises = [];
  promises.push(waitForManagerEvent("deleted"));

  let request = manager.delete(aMessageIds);
  promises.push(wrapDomRequestAsPromise(request));

  return Promise.all(promises)
    .then((aResults) => {
      return { deletedInfo: aResults[0],
               deletedFlags: aResults[1].target.result };
    });
}















function deleteMessages(aMessages) {
  let ids = messagesToIds(aMessages);
  return deleteMessagesById(ids);
}













function deleteAllMessages() {
  return getAllMessages().then(deleteMessages);
}

let pendingEmulatorCmdCount = 0;
















function runEmulatorCmdSafe(aCommand) {
  let deferred = Promise.defer();

  ++pendingEmulatorCmdCount;
  runEmulatorCmd(aCommand, function(aResult) {
    --pendingEmulatorCmdCount;

    ok(true, "Emulator response: " + JSON.stringify(aResult));
    if (Array.isArray(aResult) && aResult[0] === "OK") {
      deferred.resolve(aResult);
    } else {
      deferred.reject(aResult);
    }
  });

  return deferred.promise;
}

















function sendTextSmsToEmulator(aFrom, aText) {
  let command = "sms send " + aFrom + " " + aText;
  return runEmulatorCmdSafe(command);
}














function sendTextSmsToEmulatorAndWait(aFrom, aText) {
  let promises = [];
  promises.push(waitForManagerEvent("received"));
  promises.push(sendTextSmsToEmulator(aFrom, aText));
  return Promise.all(promises).then(aResults => aResults[0].message);
}















function sendRawSmsToEmulator(aPdu) {
  let command = "sms pdu " + aPdu;
  return runEmulatorCmdSafe(command);
}

















function sendMultipleRawSmsToEmulatorAndWait(aPdus) {
  let promises = [];

  promises.push(waitForManagerEvent("received"));
  for (let pdu of aPdus) {
    promises.push(sendRawSmsToEmulator(pdu));
  }

  return Promise.all(promises);
}








function messagesToIds(aMessages) {
  let ids = [];
  for (let message of aMessages) {
    ids.push(message.id);
  }
  return ids;
}




function compareSmsMessage(aFrom, aTo) {
  const FIELDS = ["id", "threadId", "iccId", "body", "delivery",
                  "deliveryStatus", "read", "receiver", "sender",
                  "messageClass", "timestamp", "deliveryTimestamp",
                  "sentTimestamp"];

  for (let field of FIELDS) {
    is(aFrom[field], aTo[field], "message." + field);
  }
}




function cleanUp() {
  ok(true, ":: CLEANING UP ::");

  waitFor(function() {
    SpecialPowers.flushPermissions(function() {
      ok(true, "permissions flushed");

      SpecialPowers.flushPrefEnv(function() {
        ok(true, "preferences flushed");

        finish();
      })
    });
  }, function() {
    return pendingEmulatorCmdCount === 0;
  });
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
         .then(aTestCaseMain)
         .then(cleanUp, function() {
           ok(false, 'promise rejects during test.');
           cleanUp();
         });
}










function startTestCommon(aTestCaseMain) {
  startTestBase(function() {
    return ensureMobileMessage()
      .then(deleteAllMessages)
      .then(aTestCaseMain)
      .then(deleteAllMessages);
  });
}








function runIfMultiSIM(aTest) {
  let numRIL;
  try {
    numRIL = SpecialPowers.getIntPref("ril.numRadioInterfaces");
  } catch (ex) {
    numRIL = 1;  
  }

  if (numRIL > 1) {
    return aTest();
  } else {
    log("Not a Multi-SIM environment. Test is skipped.");
    return Promise.resolve();
  }
}
