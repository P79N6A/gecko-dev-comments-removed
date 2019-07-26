


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "icc_header.js";

function setRadioEnabled(enabled) {
  SpecialPowers.addPermission("settings-write", true, document);

  
  let settings = navigator.mozSettings;
  let setLock = settings.createLock();
  let obj = {
    "ril.radio.disabled": !enabled
  };
  let setReq = setLock.set(obj);

  setReq.addEventListener("success", function onSetSuccess() {
    log("set 'ril.radio.disabled' to " + enabled);
  });

  setReq.addEventListener("error", function onSetError() {
    ok(false, "cannot set 'ril.radio.disabled' to " + enabled);
  });

  SpecialPowers.removePermission("settings-write", document);
}

function setEmulatorMccMnc(mcc, mnc) {
  let cmd = "operator set 0 Android,Android," + mcc + mnc;
  emulatorHelper.sendCommand(cmd, function (result) {
    let re = new RegExp("" + mcc + mnc + "$");
    ok(result[0].match(re), "MCC/MNC should be changed.");
  });
}


taskHelper.push(function basicTest() {
  let iccInfo = icc.iccInfo;

  is(iccInfo.iccType, "sim");
  
  
  is(iccInfo.iccid, 89014103211118510720);
  
  
  is(iccInfo.mcc, 310);
  is(iccInfo.mnc, 260);
  is(iccInfo.spn, "Android");
  
  
  is(iccInfo.msisdn, "15555215554");

  taskHelper.runNext();
});


taskHelper.push(function testDisplayConditionChange() {
  function testSPN(mcc, mnc, expectedIsDisplayNetworkNameRequired,
                   expectedIsDisplaySpnRequired, callback) {
    icc.addEventListener("iccinfochange", function handler() {
      icc.removeEventListener("iccinfochange", handler);
      is(icc.iccInfo.isDisplayNetworkNameRequired,
         expectedIsDisplayNetworkNameRequired);
      is(icc.iccInfo.isDisplaySpnRequired,
         expectedIsDisplaySpnRequired);
      
      window.setTimeout(callback, 100);
    });
    
    setEmulatorMccMnc(mcc, mnc);
  }

  let testCases = [
    
    [123, 456, false, true], 
    [234, 136,  true, true], 
    [123, 456, false, true], 
    [466,  92,  true, true], 
    [123, 456, false, true], 
    [310, 260,  true, true], 
  ];

  (function do_call(index) {
    let next = index < (testCases.length - 1) ? do_call.bind(null, index + 1) : taskHelper.runNext.bind(taskHelper);
    testCases[index].push(next);
    testSPN.apply(null, testCases[index]);
  })(0);
});


taskHelper.push(function testCardIsNotReady() {
  
  setRadioEnabled(false);
  icc.addEventListener("iccinfochange", function oniccinfochange() {
    
    if (icc.iccInfo === null) {
      icc.removeEventListener("iccinfochange", oniccinfochange);
      
      setRadioEnabled(true);
      iccManager.addEventListener("iccdetected", function oniccdetected(evt) {
        log("icc detected: " + evt.iccId);
        iccManager.removeEventListener("iccdetected", oniccdetected);
        taskHelper.runNext();
      });
    }
  });
});


taskHelper.runNext();
