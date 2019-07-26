


const {Cc: Cc, Ci: Ci, Cr: Cr, Cu: Cu} = SpecialPowers;

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;












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













function waitForManagerEvent(aEventName) {
  let deferred = Promise.defer();

  manager.addEventListener(aEventName, function onevent(aEvent) {
    manager.removeEventListener(aEventName, onevent);

    ok(true, "MobileMessageManager event '" + aEventName + "' got.");
    deferred.resolve(aEvent);
  });

  return deferred.promise;
}
















function sendSmsWithSuccess(aReceiver, aText) {
  let deferred = Promise.defer();

  let request = manager.send(aReceiver, aText);
  request.onsuccess = function(event) {
    deferred.resolve(event.target.result);
  };
  request.onerror = function(event) {
    deferred.reject(event.target.error);
  };

  return deferred.promise;
}



















function sendMmsWithFailure(aMmsParameters, aSendParameters) {
  let deferred = Promise.defer();

  let result = { message: null, error: null };
  function got(which, value) {
    result[which] = value;
    if (result.message != null && result.error != null) {
      deferred.resolve(result);
    }
  }

  manager.addEventListener("failed", function onfailed(event) {
    manager.removeEventListener("failed", onfailed);
    got("message", event.message);
  });

  let request = manager.sendMMS(aMmsParameters, aSendParameters);
  request.onsuccess = function(event) {
    deferred.reject();
  };
  request.onerror = function(event) {
    got("error", event.target.error);
  }

  return deferred.promise;
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

  let deferred = Promise.defer();

  let request = manager.delete(aMessageIds);
  request.onsuccess = function(event) {
    deferred.resolve(event.target.result);
  };
  request.onerror = deferred.reject.bind(deferred);

  return deferred.promise;
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




function cleanUp() {
  waitFor(function() {
    SpecialPowers.flushPermissions(function() {
      
      ok(true, "permissions flushed");

      finish();
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
