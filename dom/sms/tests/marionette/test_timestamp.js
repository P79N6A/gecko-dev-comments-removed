


MARIONETTE_TIMEOUT = 15000;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

let sms = window.navigator.mozSms;
let inText;
let remoteNumber = "5559997777";
let outText;
let gotSmsOnsent;
let gotReqOnsuccess;
let inSmsId = 0;
let outSmsId = 0;
let inSmsTime = 0;
let outSmsTime = 0;
let testCount = 10;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(sms, "mozSms");
  simulateIncomingSms();  
}

function simulateIncomingSms() {
  log("Simulating incoming SMS.");

  sms.onreceived = function onreceived(event) {
    log("Received 'onreceived' smsmanager event.");
    let incomingSms = event.message;
    ok(incomingSms, "incoming sms");
    ok(incomingSms.id, "sms id");
    inSmsId = incomingSms.id;
    is(incomingSms.body, inText, "msg body");
    is(incomingSms.delivery, "received", "delivery");
    is(incomingSms.read, false, "read");
    is(incomingSms.receiver, null, "receiver");
    is(incomingSms.sender, remoteNumber, "sender");
    is(incomingSms.messageClass, "normal", "messageClass");
    ok(incomingSms.timestamp instanceof Date, "timestamp is instanceof date");
    
    
    
    
    inSmsTime = Math.floor(incomingSms.timestamp.getTime() / 1000);
    log("Received SMS (id: " + inSmsId + ") timestamp: " + inSmsTime + ".");
    if(outSmsTime) {
      
      
      
      
      if(inSmsTime >= outSmsTime) {
        log("Timestamp in sms " + inSmsId + " is >= timestamp in sms "
            + outSmsId + ".");
      } else {
        log("* Timestamp in sms " + inSmsId + " is < timestamp in sms "
            + outSmsId + ".");
        ok(false, "sms timestamp is incorrect");
      }
    }
    sendSms();
  };
  
  inText = "Incoming SMS " + Date.now();
  runEmulatorCmd("sms send " + remoteNumber + " " + inText, function(result) {
    is(result[0], "OK", "emulator output");
  });
}

function sendSms() {
  log("Sending an SMS.");
  let gotSmsOnsent = false;
  let gotReqOnsuccess = false;  
  sms.onsent = function(event) {
    log("Received 'onsent' smsmanager event.");
    gotSmsOnsent = true;
    let sentSms = event.message;
    ok(sentSms, "outgoing sms");
    ok(sentSms.id, "sms id");
    outSmsId = sentSms.id;
    is(sentSms.body, outText, "msg body");
    is(sentSms.delivery, "sent", "delivery");
    is(sentSms.read, true, "read");
    is(sentSms.receiver, remoteNumber, "receiver");
    is(sentSms.sender, null, "sender");
    is(sentSms.messageClass, "normal", "messageClass");
    ok(sentSms.timestamp instanceof Date, "timestamp is instanceof date");
    
    
    
    outSmsTime = Math.round(sentSms.timestamp.getTime() / 1000);
    log("Sent SMS (id: " + outSmsId + ") timestamp: " + outSmsTime + ".");

    if (gotSmsOnsent && gotReqOnsuccess) { verifyTimeStamps(); }
  };
  outText = "Outgoing SMS " + Date.now();
  let requestRet = sms.send(remoteNumber, outText);
  ok(requestRet, "smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    gotReqOnsuccess = true;
    if(event.target.result){
      if (gotSmsOnsent && gotReqOnsuccess) { verifyTimeStamps(); }
    } else {
      log("smsrequest returned false for sms.send");
      ok(false,"SMS send failed");
      cleanUp();
    }
  };

  requestRet.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    ok(false, "sms.send request returned unexpected error: "
        + event.target.error.name );
    cleanUp();
  };
}

function verifyTimeStamps() {
  
  
  
  
  if(outSmsTime >= inSmsTime) {
    log("Timestamp in sms " + outSmsId + " is >= timestamp in sms "
        + inSmsId + ".");
  } else {
    log("* Timestamp in sms " + outSmsId + " is < timestamp in sms "
        + inSmsId + ".");
    ok(false, "sms timestamp is incorrect");
  }
  deleteMsgs();
}

function deleteMsgs() {
  log("Deleting SMS (id: " + inSmsId + ").");
  let requestRet = sms.delete(inSmsId);
  ok(requestRet,"smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    if(event.target.result){
      log("Deleting SMS (id: " + outSmsId + ").");
      let nextReqRet = sms.delete(outSmsId);
      ok(nextReqRet,"smsrequest obj returned");

      nextReqRet.onsuccess = function(event) {
        log("Received 'onsuccess' smsrequest event.");
        if(event.target.result) {
          if(--testCount) {
            simulateIncomingSms();
          } else {
            cleanUp();
          }
        } else {
          log("smsrequest returned false for sms.delete");
          ok(false,"SMS delete failed");
          cleanUp();
        }
      };
    } else {
      log("smsrequest returned false for sms.delete");
      ok(false,"SMS delete failed");
      cleanUp();
    }
  };

  requestRet.onerror = function(event) {
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
  SpecialPowers.setBoolPref("dom.sms.enabled", false);
  finish();
}


verifyInitialState();
