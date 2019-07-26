


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("sms", true, document);
SpecialPowers.setBoolPref("dom.sms.enabled", true);

let sms = window.navigator.mozSms;


let maxCharsPerSms = 160;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(sms, "mozSms");
  
  startTest();
}

function startTest() {
  
  let testData = ["", "a",
                  buildString("b", maxCharsPerSms / 2),
                  buildString("c", maxCharsPerSms - 1),
                  buildString("d", maxCharsPerSms),
                  buildString("e", maxCharsPerSms + 1),
                  buildString("f", maxCharsPerSms * 1.5),
                  buildString("g", maxCharsPerSms * 10.5)];

  
  testData.forEach(function(text){ testGetNumberOfMsgs(text); });

  
  cleanUp();
}

function buildString(char, numChars) {
  
  let string = new Array(numChars + 1).join(char);
  return string;
}

function testGetNumberOfMsgs(text) {
  
  log("getNumberOfMessagesForText length " + text.length + ".");

  if (text.length) {
    if (text.length > maxCharsPerSms) {
      expNumSms = Math.ceil(text.length / maxCharsPerSms);
    } else {
      expNumSms = 1;
    }
  } else {
    expNumSms = 0;
  }

  numMultiPartSms = sms.getNumberOfMessagesForText(text);

  if (numMultiPartSms == expNumSms) {
    log("Returned " + expNumSms + " as expected.");
    ok(true, "getNumberOfMessagesForText returned expected value");
  } else {
    log("Returned " + numMultiPartSms + " but expected " + expNumSms + ".");
    ok(false, "getNumberOfMessagesForText returned unexpected value");
  }
}

function cleanUp() {
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.clearUserPref("dom.sms.enabled");
  finish();
}


verifyInitialState();
