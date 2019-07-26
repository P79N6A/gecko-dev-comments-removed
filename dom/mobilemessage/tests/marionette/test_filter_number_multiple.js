


MARIONETTE_TIMEOUT = 20000;

SpecialPowers.addPermission("sms", true, document);
SpecialPowers.setBoolPref("dom.sms.enabled", true);

let sms = window.navigator.mozSms;
let numberMsgs = 10;
let smsList = new Array();
let defaultRemoteNumber = "+15552227777";

function verifyInitialState() {
  log("Verifying initial state.");
  ok(sms, "mozSms");
  
  deleteAllMsgs(sendSms);
}

function deleteAllMsgs(nextFunction) {
  
  let msgList = new Array();
  let filter = new MozSmsFilter;

  let request = sms.getMessages(filter, false);
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

function sendSms() {
  
  let gotSmsSent = false;
  let gotRequestSuccess = false;
  let remoteNumber = "+15558120649";
  let text = "Outgoing SMS brought to you by Firefox OS!";

  log("Sending an SMS.");

  sms.onsent = function(event) {
    log("Received 'onsent' smsmanager event.");
    gotSmsSent = true;
    log("Sent SMS (id: " + event.message.id + ").");
    if (gotSmsSent && gotRequestSuccess) {
      simulateIncomingSms();
    }
  };

  let request = sms.send(remoteNumber, text);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    if (event.target.result) {
      gotRequestSuccess = true;
      if (gotSmsSent && gotRequestSuccess) {
        simulateIncomingSms();
      }
    } else {
      log("smsrequest returned false for sms.send");
      ok(false,"SMS send failed");
      cleanUp();
    }
  };

  request.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    ok(false, "sms.send request returned unexpected error: "
        + event.target.error.name );
    cleanUp();
  };
}

function simulateIncomingSms(remoteNumber) {
  
  let text = "Incoming SMS number " + (smsList.length + 1);
  remoteNumber = typeof remoteNumber !== 'undefined'
    ? remoteNumber : defaultRemoteNumber;

  log("Simulating incoming SMS number " + (smsList.length + 1) + " of "
      + (numberMsgs - 1) + ".");

  
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
  
  let secondNumber = "+15559990000";
  if (smsList.length < (numberMsgs - 1)) {
    
    if (smsList.length % 2) {
      simulateIncomingSms(secondNumber);
    } else {
      simulateIncomingSms();
    }
  } else {
    getMsgs(secondNumber);
  }
}

function getMsgs(secondNumber) {
  
  var filter = new MozSmsFilter();
  let foundSmsList = new Array();

  
  filter.numbers = new Array(defaultRemoteNumber, secondNumber);

  log("Getting the SMS messages with numbers " + defaultRemoteNumber + " and "
      + secondNumber + ".");
  let request = sms.getMessages(filter, false);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    ok(event.target.result, "smsrequest event.target.result");
    cursor = event.target.result;

    if (cursor.message) {
      
      log("Got SMS (id: " + cursor.message.id + ").");
      
      foundSmsList.push(cursor.message);
      
      cursor.continue();
    } else {
      
      if (foundSmsList.length == smsList.length) {
        log("SMS getMessages returned " + foundSmsList.length +
            " messages as expected.");
        verifyFoundMsgs(foundSmsList);
      } else {
        log("SMS getMessages returned " + foundSmsList.length +
            " messages, but expected " + smsList.length + ".");
        ok(false, "Incorrect number of messages returned by sms.getMessages");
        deleteAllMsgs(cleanUp);
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

function verifyFoundMsgs(foundSmsList) {
  
  for (var x = 0; x < foundSmsList.length; x++) {
    is(foundSmsList[x].id, smsList[x].id, "id");
    is(foundSmsList[x].sender, smsList[x].sender, "number");
  }
  deleteAllMsgs(cleanUp);
}

function cleanUp() {
  sms.onreceived = null;
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


verifyInitialState();
