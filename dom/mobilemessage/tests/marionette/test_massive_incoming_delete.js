


MARIONETTE_TIMEOUT = 90000;
MARIONETTE_HEAD_JS = "head.js";

const SENDER = "5555552368"; 
const RECEIVER = "15555215554"; 
const MSG_TEXT = "Mozilla Firefox OS!";
const SMS_NUMBER = 100;

let SmsList = [];

function sendAllSms() {
  log("Send " + SMS_NUMBER + " SMS");

  let promises = [];

  
  promises.push(waitForManagerEvent("received", function(aEvent) {
    let message = aEvent.message;
    log("Received 'onreceived' event.");
    ok(message, "incoming sms");
    ok(message.id, "sms id");
    log("Received SMS (id: " + message.id + ").");
    ok(message.threadId, "thread id");
    is(message.body, MSG_TEXT, "msg body");
    is(message.delivery, "received", "delivery");
    is(message.deliveryStatus, "success", "deliveryStatus");
    is(message.read, false, "read");
    is(message.receiver, RECEIVER, "receiver");
    is(message.sender, SENDER, "sender");
    is(message.messageClass, "normal", "messageClass");
    is(message.deliveryTimestamp, 0, "deliveryTimestamp is 0");
    SmsList.push(message);
    return SmsList.length === SMS_NUMBER;
  }));

  
  for (let i = 0; i < SMS_NUMBER; i++) {
    promises.push(sendTextSmsToEmulator(SENDER, MSG_TEXT));
  }

  return Promise.all(promises);
}

function deleteAllSms() {
  log("Deleting SMS: " + JSON.stringify(SmsList));

  let deleteStart = Date.now();
  return deleteMessages(SmsList)
    .then(() => {
      let deleteDone = Date.now();
      log("Delete " + SmsList.length + " SMS takes " +
          (deleteDone - deleteStart) + " ms.");
    })
    
    .then(() => {
      let promises = [];

      for (let i = 0; i < SmsList.length; i++) {
        let smsId = SmsList[i].id;
        promises.push(getMessage(smsId)
          .then((aMessageInDB) => {
            log("Got SMS (id: " + aMessageInDB.id + ") but should not have.");
            ok(false, "SMS (id: " + aMessageInDB.id + ") was not deleted");
          }, (aError) => {
            log("Could not get SMS (id: " + smsId + ") as expected.");
            is(aError.name, "NotFoundError", "error returned");
          }));
      }

      return Promise.all(promises);
    });
}


startTestCommon(function testCaseMain() {
  return sendAllSms()
    .then(() => deleteAllSms());
});
