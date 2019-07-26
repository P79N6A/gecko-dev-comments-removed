


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
    
    getMsgs();
  }
}

function getMsgs() {
  var filter = new MozSmsFilter();
  let foundSmsList = new Array();

  
  let yesterday = new Date(Date.now() - 86400000); 
  let twoDaysAgo = new Date(Date.now() - 172800000);
  filter.startDate = twoDaysAgo;
  filter.endDate = yesterday;

  log("Getting SMS messages with dates between " + twoDaysAgo + " and "
      + yesterday +".");
  let request = sms.getMessages(filter, false);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    ok(event.target.result, "smsrequest event.target.result");
    cursor = event.target.result;

    if (cursor.message) {
      
      log("Got SMS (id: " + cursor.message.id + ").");
      log("SMS getMessages returned a message but should not have.");
      ok(false, "SMS date filter did not work");
    } else {
      
      log("SMS getMessages returned zero messages as expected.");
    }
    deleteAllMsgs(cleanUp);
  };

  request.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    log("sms.getMessages error: " + event.target.error.name);
    ok(false,"Could not get SMS messages");
    cleanUp();
  };
}

function cleanUp() {
  sms.onreceived = null;
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


verifyInitialState();
