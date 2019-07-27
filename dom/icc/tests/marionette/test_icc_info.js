


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "icc_header.js";

function setRadioEnabled(enabled) {
  let connection = navigator.mozMobileConnections[0];
  ok(connection);

  let request  = connection.setRadioEnabled(enabled);

  request.onsuccess = function onsuccess() {
    log('setRadioEnabled: ' + enabled);
  };

  request.onerror = function onerror() {
    ok(false, "setRadioEnabled should be ok");
  };
}

function setEmulatorMccMnc(mcc, mnc) {
  let cmd = "operator set 0 Android,Android," + mcc + mnc;
  emulatorHelper.sendCommand(cmd, function(result) {
    let re = new RegExp("" + mcc + mnc + "$");
    ok(result[0].match(re), "MCC/MNC should be changed.");
  });
}


taskHelper.push(function basicTest() {
  let iccInfo = icc.iccInfo;

  
  
  is(iccInfo.iccid, 89014103211118510720);

  if (iccInfo instanceof Ci.nsIDOMMozGsmIccInfo) {
    log("Test Gsm IccInfo");
    is(iccInfo.iccType, "sim");
    is(iccInfo.spn, "Android");
    
    
    is(iccInfo.mcc, 310);
    is(iccInfo.mnc, 260);
    
    
    is(iccInfo.msisdn, "15555215554");
  } else {
    log("Test Cdma IccInfo");
    is(iccInfo.iccType, "ruim");
    
    
    
    is(iccInfo.mdn, "8587777777");
    
    
    
    is(iccInfo.prlVersion, 1);
  }

  taskHelper.runNext();
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
