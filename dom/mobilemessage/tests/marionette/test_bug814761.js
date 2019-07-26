


MARIONETTE_TIMEOUT = 60000;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

let manager = window.navigator.mozMobileMessage;
ok(manager instanceof MozMobileMessageManager,
   "manager is instance of " + manager.constructor);






let fromNumber = "5551110000";
let msgLength = 379;
let msgText = new Array(msgLength + 1).join('a');

let pendingEmulatorCmdCount = 0;
function sendSmsToEmulator(from, text) {
  ++pendingEmulatorCmdCount;

  let cmd = "sms send " + from + " " + text;
  runEmulatorCmd(cmd, function (result) {
    --pendingEmulatorCmdCount;

    is(result[0], "OK", "Emulator response");
  });
}

function firstIncomingSms() {
  simulateIncomingSms(secondIncomingSms);
}

function secondIncomingSms() {
  simulateIncomingSms(cleanUp);
}

function simulateIncomingSms(nextFunction) {
  log("Simulating incoming multipart SMS (" + msgText.length
      + " chars total).");

  manager.onreceived = function onreceived(event) {
    log("Received 'onreceived' event.");
    manager.onreceived = null;

    let incomingSms = event.message;
    ok(incomingSms, "incoming sms");
    is(incomingSms.body, msgText, "msg body");

    window.setTimeout(nextFunction, 0);
  };

  sendSmsToEmulator(fromNumber, msgText);
}

function cleanUp() {
  if (pendingEmulatorCmdCount) {
    window.setTimeout(cleanUp, 100);
    return;
  }

  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


firstIncomingSms();
