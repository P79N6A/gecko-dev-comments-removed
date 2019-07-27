


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");





let RSM;

let gReceivedMsgType = null;
let gReceivedMessage = null;






function newRILSystemMessenger() {
  if (!RSM) {
    RSM = Cu.import("resource://gre/modules/RILSystemMessenger.jsm", {});
    equal(typeof RSM.RILSystemMessenger, "function", "RSM.RILSystemMessenger");
  }

  let rsm = new RSM.RILSystemMessenger();
  rsm.broadcastMessage = (aType, aMessage) => {
    gReceivedMsgType = aType;
    gReceivedMessage = aMessage;
  };

  return rsm;
}

function equal_received_system_message(aType, aMessage) {
  equal(aType, gReceivedMsgType);
  deepEqual(aMessage, gReceivedMessage);
  gReceivedMsgType = null;
  gReceivedMessage = null;
}




function run_test() {
  let telephonyMessenger = Cc["@mozilla.org/ril/system-messenger-helper;1"]
                           .getService(Ci.nsITelephonyMessenger);

  let smsMessenger = Cc["@mozilla.org/ril/system-messenger-helper;1"]
                     .getService(Ci.nsISmsMessenger);

  ok(telephonyMessenger !== null, "Get TelephonyMessenger.");
  ok(smsMessenger != null, "Get SmsMessenger.");

  run_next_test();
}




add_test(function test_telephony_messenger_notify_new_call() {
  let messenger = newRILSystemMessenger();

  messenger.notifyNewCall();
  equal_received_system_message("telephony-new-call", {});

  run_next_test();
});




add_test(function test_telephony_messenger_notify_call_ended() {
  let messenger = newRILSystemMessenger();

  messenger.notifyCallEnded(1,
                            "+0987654321",
                            null,
                            true,
                            500,
                            false,
                            true);

  equal_received_system_message("telephony-call-ended", {
    serviceId: 1,
    number: "+0987654321",
    emergency: true,
    duration: 500,
    direction: "incoming",
    hangUpLocal: true
  });

  
  messenger.notifyCallEnded(1,
                            "+0987654321",
                            "+1234567890",
                            true,
                            500,
                            true,
                            false);

  equal_received_system_message("telephony-call-ended", {
    serviceId: 1,
    number: "+0987654321",
    emergency: true,
    duration: 500,
    direction: "outgoing",
    hangUpLocal: false,
    secondNumber: "+1234567890"
  });

  run_next_test();
});




add_test(function test_sms_messenger_notify_sms() {
  let messenger = newRILSystemMessenger();
  let timestamp = Date.now();
  let sentTimestamp = timestamp + 100;
  let deliveryTimestamp = sentTimestamp + 100;

  
  messenger.notifySms(Ci.nsISmsMessenger.NOTIFICATION_TYPE_RECEIVED,
                      1,
                      2,
                      "99887766554433221100",
                      Ci.nsISmsService.DELIVERY_TYPE_RECEIVED,
                      Ci.nsISmsService.DELIVERY_STATUS_TYPE_SUCCESS,
                      "+0987654321",
                      null,
                      "Incoming message",
                      Ci.nsISmsService.MESSAGE_CLASS_TYPE_CLASS_2,
                      timestamp,
                      sentTimestamp,
                      0,
                      false);

  equal_received_system_message("sms-received", {
      iccId:             "99887766554433221100",
      type:              "sms",
      id:                1,
      threadId:          2,
      delivery:          "received",
      deliveryStatus:    "success",
      sender:            "+0987654321",
      receiver:          null,
      body:              "Incoming message",
      messageClass:      "class-2",
      timestamp:         timestamp,
      sentTimestamp:     sentTimestamp,
      deliveryTimestamp: 0,
      read:              false
    });

  
  messenger.notifySms(Ci.nsISmsMessenger.NOTIFICATION_TYPE_SENT,
                      3,
                      4,
                      "99887766554433221100",
                      Ci.nsISmsService.DELIVERY_TYPE_SENT,
                      Ci.nsISmsService.DELIVERY_STATUS_TYPE_PENDING,
                      null,
                      "+0987654321",
                      "Outgoing message",
                      Ci.nsISmsService.MESSAGE_CLASS_TYPE_NORMAL,
                      timestamp,
                      0,
                      0,
                      true);

  equal_received_system_message("sms-sent", {
      iccId:             "99887766554433221100",
      type:              "sms",
      id:                3,
      threadId:          4,
      delivery:          "sent",
      deliveryStatus:    "pending",
      sender:            null,
      receiver:          "+0987654321",
      body:              "Outgoing message",
      messageClass:      "normal",
      timestamp:         timestamp,
      sentTimestamp:     0,
      deliveryTimestamp: 0,
      read:              true
    });

  
  messenger.notifySms(Ci.nsISmsMessenger.NOTIFICATION_TYPE_DELIVERY_SUCCESS,
                      5,
                      6,
                      "99887766554433221100",
                      Ci.nsISmsService.DELIVERY_TYPE_SENT,
                      Ci.nsISmsService.DELIVERY_STATUS_TYPE_SUCCESS,
                      null,
                      "+0987654321",
                      "Outgoing message",
                      Ci.nsISmsService.MESSAGE_CLASS_TYPE_NORMAL,
                      timestamp,
                      0,
                      deliveryTimestamp,
                      true);

  equal_received_system_message("sms-delivery-success", {
      iccId:             "99887766554433221100",
      type:              "sms",
      id:                5,
      threadId:          6,
      delivery:          "sent",
      deliveryStatus:    "success",
      sender:            null,
      receiver:          "+0987654321",
      body:              "Outgoing message",
      messageClass:      "normal",
      timestamp:         timestamp,
      sentTimestamp:     0,
      deliveryTimestamp: deliveryTimestamp,
      read:              true
    });

  
  try {
    messenger.notifySms(3,
                        1,
                        2,
                        "99887766554433221100",
                        Ci.nsISmsService.DELIVERY_TYPE_RECEIVED,
                        Ci.nsISmsService.DELIVERY_STATUS_TYPE_SUCCESS,
                        "+0987654321",
                        null,
                        "Incoming message",
                        Ci.nsISmsService.MESSAGE_CLASS_TYPE_NORMAL,
                        timestamp,
                        sentTimestamp,
                        0,
                        false);
    ok(false, "Failed to verify the protection of invalid nsISmsMessenger.NOTIFICATION_TYPE!");
  } catch (e) {}

  run_next_test();
});
