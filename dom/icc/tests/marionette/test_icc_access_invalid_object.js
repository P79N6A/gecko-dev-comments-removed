


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head.js";

function testInvalidIccObject(aIcc) {
  
  try {
    is(aIcc.iccInfo, null, "iccInfo: expect to get null");
  } catch(e) {
    ok(false, "access iccInfo should not get exception");
  }

  
  try {
    is(aIcc.cardState, null, "cardState: expect to get null");
  } catch(e) {
    ok(false, "access cardState should not get exception");
  }

  
  try {
    aIcc.sendStkResponse({}, {});
    ok(false, "sendStkResponse() should get exception");
  } catch(e) {}
  try {
    aIcc.sendStkMenuSelection(0, false);
    ok(false, "sendStkMenuSelection() should get exception");
  } catch(e) {}
  try {
    aIcc.sendStkTimerExpiration({});
    ok(false, "sendStkTimerExpiration() should get exception");
  } catch(e) {}
  try {
    aIcc.sendStkEventDownload({});
    ok(false, "sendStkEventDownload() should get exception");
  } catch(e) {}

  
  try {
    aIcc.getCardLock("pin");
    ok(false, "getCardLock() should get exception");
  } catch(e) {}
  try {
    aIcc.unlockCardLock({});
    ok(false, "unlockCardLock() should get exception");
  } catch(e) {}
  try {
    aIcc.setCardLock({});
    ok(false, "setCardLock() should get exception");
  } catch(e) {}
  try {
    aIcc.getCardLockRetryCount("pin");
    ok(false, "getCardLockRetryCount() should get exception");
  } catch(e) {}

  
  try {
    aIcc.readContacts("adn");
    ok(false, "readContacts() should get exception");
  } catch(e) {}
  try {
    aIcc.updateContact("adn", {});
    ok(false, "updateContact() should get exception");
  } catch(e) {}

  
  try {
    aIcc.matchMvno("imsi");
    ok(false, "matchMvno() should get exception");
  } catch(e) {}

  
  return aIcc.getServiceState("fdn").then(() => {
    ok(false, "getServiceState() should be rejected");
  }, () => {});
}


startTestCommon(function() {
  let icc = getMozIcc();

  return Promise.resolve()
    
    .then(() => {
      let promises = [];
      promises.push(setRadioEnabled(false));
      promises.push(waitForTargetEvent(iccManager, "iccundetected"));
      return Promise.all(promises);
    })
    
    .then(() => testInvalidIccObject(icc))
    
    .then(() => {
      let promises = [];
      promises.push(setRadioEnabled(true));
      promises.push(waitForTargetEvent(iccManager, "iccdetected"));
      return Promise.all(promises);
    });
});
