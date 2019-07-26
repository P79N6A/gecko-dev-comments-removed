


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

  
  
  is(iccInfo.iccid, 89014103211118510720);

  
  
  is(iccInfo.mcc, 310);
  is(iccInfo.mnc, 260);
  is(iccInfo.spn, "Android");
  
  
  is(iccInfo.msisdn, "15555215554");

  testDisplayConditionChange(testSPN, [
    
    [123, 456, false, true], 
    [234, 136,  true, true], 
    [123, 456, false, true], 
    [466,  92,  true, true], 
    [123, 456, false, true], 
    [310, 260,  true, true], 
  ], finalize);
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

function waitForIccInfoChange(callback) {
  icc.addEventListener("iccinfochange", function handler() {
    icc.removeEventListener("iccinfochange", handler);
    callback();
  });
}

function finalize() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
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
