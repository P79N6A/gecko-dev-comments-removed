


MARIONETTE_TIMEOUT = 60000;

const PDU_SMSC = "00"; 
const PDU_FIRST_OCTET = "00"; 
const PDU_SENDER = "0191F1"; 
const PDU_PID_NORMAL = "00";
const PDU_PID_ANSI_136_R_DATA = "7C";
const PDU_PID_USIM_DATA_DOWNLOAD = "7F";
const PDU_TIMESTAMP = "00101000000000"; 
const PDU_UDL = "01";
const PDU_UD = "41";

SpecialPowers.addPermission("sms", true, document);

let sms = window.navigator.mozSms;
ok(sms instanceof MozSmsManager);

let pendingEmulatorCmdCount = 0;
function sendSmsPduToEmulator(pdu) {
  ++pendingEmulatorCmdCount;

  let cmd = "sms pdu " + pdu;
  runEmulatorCmd(cmd, function (result) {
    --pendingEmulatorCmdCount;

    is(result[0], "OK", "Emulator response");
  });
}

const TIMESTAMP = Date.UTC(2000, 0, 1);
function checkMessage(message, id, threadId, messageClass) {
  ok(message instanceof MozSmsMessage,
     "message is instanceof " + message.constructor);
  if (id == null) {
    ok(message.id > 0, "message.id");
  } else {
    is(message.id, id, "message.id");
  }
  if (threadId == null) {
    ok(message.threadId > 0, "message.threadId");
  } else {
    is(message.threadId, threadId, "message.threadId");
  }
  is(message.delivery, "received", "message.delivery");
  is(message.deliveryStatus, "success", "message.deliveryStatus");
  is(message.sender, "+1", "message.sender");
  is(message.body, "A", "message.body");
  is(message.messageClass, messageClass, "message.messageClass");
  ok(message.timestamp instanceof Date,
     "message.timestamp is instanceof " + message.timestamp.constructor);
  is(message.timestamp.getTime(), TIMESTAMP, "message.timestamp");
  is(message.read, false, "message.read");
}

function test_message_class_0() {
  let allDCSs = [
    "10", 
    "50", 
    "F0"  
  ];

  function do_test(dcsIndex) {
    sms.addEventListener("received", function onReceived(event) {
      sms.removeEventListener("received", onReceived);

      let message = event.message;
      checkMessage(message, -1, 0, "class-0");

      
      let cursor = sms.getMessages(null, false);
      cursor.onsuccess = function onsuccess() {
        if (cursor.result) {
          
          isnot(cursor.result.sender, message.sender, "cursor.result.sender");

          cursor.continue();
          return;
        }

        
        ++dcsIndex;
        if (dcsIndex >= allDCSs.length) {
          window.setTimeout(test_message_class_1, 0);
        } else {
          window.setTimeout(do_test.bind(null, dcsIndex), 0);
        }
      };
      cursor.onerror = function onerror() {
        ok(false, "Can't fetch messages from SMS database");
      };
    });

    let dcs = allDCSs[dcsIndex];
    log("  Testing DCS " + dcs);
    let pdu = PDU_SMSC + PDU_FIRST_OCTET + PDU_SENDER + PDU_PID_NORMAL +
              dcs + PDU_TIMESTAMP + PDU_UDL + PDU_UD;

    sendSmsPduToEmulator(pdu);
  }

  log("Checking Message Class 0");
  do_test(0);
}

function doTestMessageClassGeneric(allDCSs, messageClass, next) {
  function do_test(dcsIndex) {
    sms.addEventListener("received", function onReceived(event) {
      sms.removeEventListener("received", onReceived);

      
      checkMessage(event.message, null, null, messageClass);

      ++dcsIndex;
      if (dcsIndex >= allDCSs.length) {
        window.setTimeout(next, 0);
      } else {
        window.setTimeout(do_test.bind(null, dcsIndex), 0);
      }
    });

    let dcs = allDCSs[dcsIndex];
    log("  Testing DCS " + dcs);
    let pdu = PDU_SMSC + PDU_FIRST_OCTET + PDU_SENDER + PDU_PID_NORMAL +
              dcs + PDU_TIMESTAMP + PDU_UDL + PDU_UD;

    sendSmsPduToEmulator(pdu);
  }

  do_test(0);
}

function test_message_class_1() {
  let allDCSs = [
    "11", 
    "51", 
    "F1"  
  ];

  log("Checking Message Class 1");
  doTestMessageClassGeneric(allDCSs, "class-1", test_message_class_2);
}

function test_message_class_2() {
  let allDCSs = [
    "12", 
    "52", 
    "F2"  
  ];

  let allPIDs = [
    PDU_PID_NORMAL,
    PDU_PID_ANSI_136_R_DATA,
    PDU_PID_USIM_DATA_DOWNLOAD
  ];

  function do_test_dcs(dcsIndex) {
    function do_test_pid(pidIndex) {
      function onReceived(event) {
        if (pidIndex == 0) {
          
          checkMessage(event.message, null, null, "class-2");

          next();
          return;
        }

        
        
        
        
        
        ok(false, "SMS-PP messages shouldn't be sent to content");
      }

      function next() {
        sms.removeEventListener("received", onReceived);

        ++pidIndex;
        if (pidIndex >= allPIDs.length) {
          ++dcsIndex;
          if (dcsIndex >= allDCSs.length) {
            window.setTimeout(test_message_class_3, 0);
          } else {
            window.setTimeout(do_test_dcs.bind(null, dcsIndex), 0);
          }
        } else {
          window.setTimeout(do_test_pid.bind(null, pidIndex), 0);
        }
      }

      sms.addEventListener("received", onReceived);

      if (pidIndex != 0) {
        
        window.setTimeout(next, 3000);
      }

      let pid = allPIDs[pidIndex];
      log("    Testing PID " + pid);

      let pdu = PDU_SMSC + PDU_FIRST_OCTET + PDU_SENDER + pid + dcs +
                PDU_TIMESTAMP + PDU_UDL + PDU_UD;

      sendSmsPduToEmulator(pdu);
    }

    let dcs = allDCSs[dcsIndex];
    log("  Testing DCS " + dcs);

    do_test_pid(0);
  }

  log("Checking Message Class 2");
  do_test_dcs(0);
}

function test_message_class_3() {
  let allDCSs = [
    "13", 
    "53", 
    "F3"  
  ];

  log("Checking Message Class 3");
  doTestMessageClassGeneric(allDCSs, "class-3", cleanUp);
}

function cleanUp() {
  if (pendingEmulatorCmdCount) {
    window.setTimeout(cleanUp, 100);
    return;
  }

  SpecialPowers.removePermission("sms", document);
  finish();
}

test_message_class_0();
