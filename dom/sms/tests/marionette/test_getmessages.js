


MARIONETTE_TIMEOUT = 20000;

SpecialPowers.addPermission("sms", true, document);
SpecialPowers.setBoolPref("dom.sms.enabled", true);

let sms = window.navigator.mozSms;
let numberMsgs = 10;
let smsList = new Array();

function verifyInitialState() {
  log("Verifying initial state.");
  ok(sms, "mozSms");
  
  deleteAllMsgs(simulateIncomingSms);
}

function isIn(aVal, aArray, aMsg) {
  ok(aArray.indexOf(aVal) >= 0, aMsg);
}

function deleteAllMsgs(nextFunction) {
  let msgList = new Array();
  let smsFilter = new MozSmsFilter;

  let request = sms.getMessages(smsFilter, false);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    ok(event.target.result, "smsrequest event.target.result");
    cursor = event.target.result;
    
    if (cursor.message) {
      msgList.push(cursor.message.id);
      
      cursor.continue();
    } else {
      
      if (msgList.length) {
        log("Found " + msgList.length + " SMS messages to delete.");
        deleteMsgs(msgList, nextFunction);
      } else {
        log("No SMS messages found.");
        nextFunction();
      }
    }
  };

  request.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    log("sms.getMessages error: " + event.target.error.name);
    ok(false,"Could not get SMS messages");
    cleanUp();
  };
}

function deleteMsgs(msgList, nextFunction) {
  let smsId = msgList.shift();

  log("Deleting SMS (id: " + smsId + ").");
  let request = sms.delete(smsId);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    if (event.target.result) {
      
      if (msgList.length) {
        deleteMsgs(msgList, nextFunction);
      } else {
        log("Finished deleting SMS messages.");
        nextFunction();
      }
    } else {
      log("SMS delete failed.");
      ok(false,"sms.delete request returned false");
      cleanUp();
    }
  };

  request.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    ok(false, "sms.delete request returned unexpected error: "
        + event.target.error.name );
    cleanUp();
  };
}

function simulateIncomingSms() {
  let text = "Incoming SMS number " + (smsList.length + 1);
  let remoteNumber = "5552229797";

  log("Simulating incoming SMS number " + (smsList.length + 1) + " of "
      + numberMsgs + ".");

  
  rcvdEmulatorCallback = false;
  runEmulatorCmd("sms send " + remoteNumber + " " + text, function(result) {
    is(result[0], "OK", "emulator callback");
    rcvdEmulatorCallback = true;
  });
}


sms.onreceived = function onreceived(event) {
  log("Received 'onreceived' sms event.");
  let incomingSms = event.message;
  log("Received SMS (id: " + incomingSms.id + ").");

  
  smsList.push(incomingSms);

  
  waitFor(nextRep,function() {
    return(rcvdEmulatorCallback);
  });
};

function nextRep() {
  if (smsList.length < numberMsgs) {
    simulateIncomingSms();
  } else {
    log("Received " + numberMsgs + " sms messages in total.");
    getMsgs(false);
  }
}

function getMsgs(reverse) {
  let smsFilter = new MozSmsFilter;
  let foundSmsCount = 0;
  let foundSmsList = new Array();

  if (!reverse) {
    log("Getting the sms messages.");
  } else {
    log("Getting the sms messages in reverse order.");
  }

  
  
  let request = sms.getMessages(smsFilter, reverse);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    ok(event.target.result, "smsrequest event.target.result");
    cursor = event.target.result;

    if (cursor.message) {
      
      log("Got SMS (id: " + cursor.message.id + ").");
      foundSmsCount++;
      
      foundSmsList.push(cursor.message);
      
      cursor.continue();
    } else {
      
      if (foundSmsCount == numberMsgs) {
        log("SMS getMessages returned " + foundSmsCount +
            " messages as expected.");  
      } else {
        log("SMS getMessages returned " + foundSmsCount +
            " messages, but expected " + numberMsgs + ".");
        ok(false, "Incorrect number of messages returned by sms.getMessages");
      }
      verifyFoundMsgs(foundSmsList, reverse);
    }
  };

  request.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    log("sms.getMessages error: " + event.target.error.name);
    ok(false,"Could not get SMS messages");
    cleanUp();
  };
}

function verifyFoundMsgs(foundSmsList, reverse) {
  if (reverse) {
    smsList.reverse();
  }
  for (var x = 0; x < numberMsgs; x++) {
    is(foundSmsList[x].id, smsList[x].id, "id");
    is(foundSmsList[x].body, smsList[x].body, "body");
    is(foundSmsList[x].delivery, smsList[x].delivery, "delivery");
    is(foundSmsList[x].read, smsList[x].read, "read");

    
    
    if (!smsList[x].receiver) {
      isIn(foundSmsList[x].receiver, ["15555215554", "+15555215554"], "receiver");
    } else {
      isIn(foundSmsList[x].receiver, [smsList[x].receiver, "+15555215554"], "receiver");
    }

    isIn(foundSmsList[x].sender, [smsList[x].sender, "+15552229797"], "sender");
    is(foundSmsList[x].timestamp.getTime(), smsList[x].timestamp.getTime(),
        "timestamp");
  }

  log("Content in all of the returned SMS messages is correct.");

  if (!reverse) {
    
    getMsgs(true);
  } else {
    
    deleteAllMsgs(cleanUp);
  };
}

function cleanUp() {
  sms.onreceived = null;
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


verifyInitialState();
