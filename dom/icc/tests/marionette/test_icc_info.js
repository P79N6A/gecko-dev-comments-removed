


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";


function basicTest(aIcc) {
  let iccInfo = aIcc.iccInfo;

  
  
  is(iccInfo.iccid, 89014103211118510720);

  if (iccInfo instanceof MozGsmIccInfo) {
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
}


startTestCommon(function() {
  let icc = getMozIcc();

  return Promise.resolve()
    
    .then(() => basicTest(icc))
    
    .then(() => {
      let promises = [];
      promises.push(setRadioEnabled(false));
      promises.push(waitForTargetEvent(icc, "iccinfochange", function() {
        
        return icc.iccInfo === null;
      }));
      return Promise.all(promises);
    })
    
    .then(() => {
      let promises = [];
      promises.push(setRadioEnabled(true));
      promises.push(waitForTargetEvent(iccManager, "iccdetected"));
      return Promise.all(promises);
    });
});
