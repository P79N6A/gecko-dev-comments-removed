


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


taskHelper.push(function testAccessRemovedIccObject() {
  setRadioEnabled(false);
  iccManager.addEventListener("iccundetected", function oniccundetected(evt) {
    log("got icc undetected event");
    iccManager.removeEventListener("iccundetected", oniccundetected);
    is(evt.iccId, iccId, "icc " + evt.iccId + " becomes undetected");

    
    try {
      is(icc.iccInfo, null, "iccInfo: expect to get null");
    } catch(e) {
      ok(false, "access iccInfo should not get exception");
    }

    
    try {
      is(icc.cardState, null, "cardState: expect to get null");
    } catch(e) {
      ok(false, "access cardState should not get exception");
    }

    
    try {
      icc.sendStkResponse({}, {});
      ok(false, "sendStkResponse() should get exception");
    } catch(e) {}
    try {
      icc.sendStkMenuSelection(0, false);
      ok(false, "sendStkMenuSelection() should get exception");
    } catch(e) {}
    try {
      icc.sendStkTimerExpiration({});
      ok(false, "sendStkTimerExpiration() should get exception");
    } catch(e) {}
    try {
      icc.sendStkEventDownload({});
      ok(false, "sendStkEventDownload() should get exception");
    } catch(e) {}

    
    try {
      icc.getCardLock("");
      ok(false, "getCardLock() should get exception");
    } catch(e) {}
    try {
      icc.unlockCardLock({});
      ok(false, "unlockCardLock() should get exception");
    } catch(e) {}
    try {
      icc.setCardLock({});
      ok(false, "setCardLock() should get exception");
    } catch(e) {}
    try {
      icc.getCardLockRetryCount("");
      ok(false, "getCardLockRetryCount() should get exception");
    } catch(e) {}

    
    try {
      icc.readContacts("");
      ok(false, "readContacts() should get exception");
    } catch(e) {}
    try {
      icc.updateContact("", {});
      ok(false, "updateContact() should get exception");
    } catch(e) {}

    
    try {
      icc.iccOpenChannel("");
      ok(false, "iccOpenChannel() should get exception");
    } catch(e) {}
    try {
      icc.iccExchangeAPDU(0, {});
      ok(false, "iccExchangeAPDU() should get exception");
    } catch(e) {}
    try {
      icc.iccCloseChannel(0);
      ok(false, "iccCloseChannel() should get exception");
    } catch(e) {}

    
    setRadioEnabled(true);
    iccManager.addEventListener("iccdetected", function oniccdetected(evt) {
      iccManager.removeEventListener("iccdetected", oniccdetected);
      taskHelper.runNext();
    });
  });
});


taskHelper.runNext();
