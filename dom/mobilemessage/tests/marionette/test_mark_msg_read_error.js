


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("sms", true, document);
SpecialPowers.setBoolPref("dom.sms.enabled", true);

let sms = window.navigator.mozSms;
let smsId;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(sms, "mozSms");
  simulateIncomingSms();
}

function simulateIncomingSms() {
  let text = "Incoming SMS courtesy of Firefox OS";
  let remoteNumber = "5557779999";

  log("Simulating incoming SMS.");

  
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
  is(incomingSms.read, false, "incoming message read");
  smsId = incomingSms.id;

  
  waitFor(test1, function() {
    return(rcvdEmulatorCallback);
  });
};

function markMsgError(invalidId, readBool, nextFunction) {
  let requestRet = sms.markMessageRead(invalidId, readBool);
  ok(requestRet, "smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event, but expected error.");
    ok(false, "Smsrequest should have returned error but did not");
    nextFunction();
  };

  requestRet.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    is(event.target.error.name, "NotFoundError", "error returned");
    nextFunction();
  };
}

function test1() {
  
  let msgIdNoExist = smsId + 1;
  log("Attempting to mark non-existent sms (id: " + msgIdNoExist
      + ") read, expect error.");
  markMsgError(msgIdNoExist, true, test2);
}

function test2() {
  
  invalidId = -1;
  log("Attempting to mark sms unread using an invalid id (id: " + invalidId
      + "), expect error.");
  markMsgError(invalidId, false, deleteMsg);
}

function deleteMsg() {
  log("Deleting SMS (id: " + smsId + ").");
  let request = sms.delete(smsId);
  ok(request instanceof MozSmsRequest,
      "request is instanceof " + request.constructor);

  request.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    if (event.target.result) {
      
      cleanUp();
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

function cleanUp() {
  sms.onreceived = null;
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


verifyInitialState();
