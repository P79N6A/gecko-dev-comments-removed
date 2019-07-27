


MARIONETTE_TIMEOUT = 20000;
MARIONETTE_HEAD_JS = "head.js";



startTestCommon(function() {
  let icc = getMozIcc();

  
  

  
  
  
  return Promise.resolve()
    .then(() => icc.sendStkTimerExpiration({ timerId: 5, timerValue: 1234567 / 1000 }))
    .then(() => verifyWithPeekedStkEnvelope(
      "D7" + 
      "0C" + 
      "82028281" + 
      "A40105" + 
      "A503000243" 
    ));
});