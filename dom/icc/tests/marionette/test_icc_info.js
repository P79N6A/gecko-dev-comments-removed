


MARIONETTE_TIMEOUT = 30000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let icc;
let iccInfo;
ifr.onload = function() {
  icc = ifr.contentWindow.navigator.mozIccManager;
  ok(icc instanceof ifr.contentWindow.MozIccManager,
     "icc is instanceof " + icc.constructor);

  iccInfo = icc.iccInfo;

  is(iccInfo.iccType, "sim");

  
  
  is(iccInfo.iccid, 89014103211118510720);

  
  
  is(iccInfo.mcc, 310);
  is(iccInfo.mnc, 260);
  is(iccInfo.spn, "Android");
  
  
  is(iccInfo.msisdn, "15555215554");

  runNextTest();
};
document.body.appendChild(ifr);

let emulatorCmdPendingCount = 0;
function sendEmulatorCommand(cmd, callback) {
  emulatorCmdPendingCount++;
  runEmulatorCmd(cmd, function (result) {
    emulatorCmdPendingCount--;
    is(result[result.length - 1], "OK");
    callback(result);
  });
}

function setEmulatorMccMnc(mcc, mnc) {
  let cmd = "operator set 0 Android,Android," + mcc + mnc;
  sendEmulatorCommand(cmd, function (result) {
    let re = new RegExp("" + mcc + mnc + "$");
    ok(result[0].match(re), "MCC/MNC should be changed.");
  });
}

function setAirplaneModeEnabled(enabled) {
  let settings = ifr.contentWindow.navigator.mozSettings;
  let setLock = settings.createLock();
  let obj = {
    "ril.radio.disabled": enabled
  };
  let setReq = setLock.set(obj);

  log("set airplane mode to " + enabled);

  setReq.addEventListener("success", function onSetSuccess() {
    log("set 'ril.radio.disabled' to " + enabled);
  });

  setReq.addEventListener("error", function onSetError() {
    ok(false, "cannot set 'ril.radio.disabled' to " + enabled);
  });
}

function waitForIccInfoChange(callback) {
  icc.addEventListener("iccinfochange", function handler() {
    icc.removeEventListener("iccinfochange", handler);
    callback();
  });
}

function waitForCardStateChange(expectedCardState, callback) {
  icc.addEventListener("cardstatechange", function oncardstatechange() {
    log("card state changes to " + icc.cardState);
    if (icc.cardState === expectedCardState) {
      log("got expected card state: " + icc.cardState);
      icc.removeEventListener("cardstatechange", oncardstatechange);
      callback();
    }
  });
}


function testDisplayConditionChange(func, caseArray, oncomplete) {
  (function do_call(index) {
    let next = index < (caseArray.length - 1) ? do_call.bind(null, index + 1) : oncomplete;
    caseArray[index].push(next);
    func.apply(null, caseArray[index]);
  })(0);
}

function testSPN(mcc, mnc, expectedIsDisplayNetworkNameRequired,
                  expectedIsDisplaySpnRequired, callback) {
  waitForIccInfoChange(function() {
    is(iccInfo.isDisplayNetworkNameRequired,
       expectedIsDisplayNetworkNameRequired);
    is(iccInfo.isDisplaySpnRequired,
       expectedIsDisplaySpnRequired);
    
    window.setTimeout(callback, 100);
  });
  setEmulatorMccMnc(mcc, mnc);
}


function testCardIsNotReady() {
  
  setAirplaneModeEnabled(true);

  waitForCardStateChange(null, function callback() {
    is(icc.iccInfo, null);

    
    setAirplaneModeEnabled(false);
    waitForCardStateChange("ready", runNextTest);
  });
}

let tests = [
  testDisplayConditionChange.bind(this, testSPN, [
    
    [123, 456, false, true], 
    [234, 136,  true, true], 
    [123, 456, false, true], 
    [466,  92,  true, true], 
    [123, 456, false, true], 
    [310, 260,  true, true], 
  ], runNextTest),
  testCardIsNotReady
];

function runNextTest() {
  let test = tests.shift();
  if (!test) {
    finalize();
    return;
  }

  test();
}

function finalize() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
}
