


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

  ok(telephonyMessenger !== null, "Get TelephonyMessenger.");

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
