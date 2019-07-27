


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";


startTestCommon(function() {
  let icc = getMozIcc();

  
  is(icc.cardState, "ready", "card state is " + icc.cardState);

  
  return Promise.resolve()
    
    .then(() => {
      let promises = [];
      promises.push(setRadioEnabled(false));
      promises.push(waitForTargetEvent(icc, "cardstatechange", function() {
        return icc.cardState === null;
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
