


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













function sendMmsWithFailure(aMmsParameters) {
  let deferred = Promise.defer();

  manager.onfailed = function(event) {
    manager.onfailed = null;
    deferred.resolve(event.message);
  };

  let request = manager.sendMMS(aMmsParameters);
  request.onsuccess = function(event) {
    deferred.reject();
  };

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
        if (thread.id == aThreadId) {
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







function messagesToIds(aMessages) {
  let ids = [];
  for (let message of aMessages) {
    ids.push(message.id);
  }
  return ids;
}



function cleanUp() {
  SpecialPowers.flushPermissions(function() {
    
    ok(true, "permissions flushed");

    finish();
  });
}

function startTestCommon(aTestCaseMain) {
  ensureMobileMessage()
    .then(deleteAllMessages)
    .then(aTestCaseMain)
    .then(deleteAllMessages)
    .then(cleanUp, cleanUp);
}
