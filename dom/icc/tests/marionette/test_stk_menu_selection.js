


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";



startTestCommon(function() {
  let icc = getMozIcc();

  
  

  
  
  
  return Promise.resolve()
    .then(() => icc.sendStkMenuSelection(1, true))
    .then(() => verifyWithPeekedStkEnvelope(
      "D3" + 
      "09" + 
      "82020181" + 
      "900101" + 
      "9500" 
    ))

    .then(() => icc.sendStkMenuSelection(0, false))
    .then(() => verifyWithPeekedStkEnvelope(
      "D3" + 
      "07" + 
      "82020181" + 
      "900100" 
    ));
});