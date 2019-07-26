


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


taskHelper.push(function basicTest() {
  is(icc.cardState, "ready", "card state is " + icc.cardState);
  taskHelper.runNext();
});


taskHelper.push(function testCardStateChange() {
  
  setRadioEnabled(false);
  icc.addEventListener("cardstatechange", function oncardstatechange() {
    log("card state changes to " + icc.cardState);
    
    if (icc.cardState === null) {
      icc.removeEventListener("cardstatechange", oncardstatechange);
      
      setRadioEnabled(true);
      icc.addEventListener("cardstatechange", function oncardstatechange(evt) {
        log("card state changes to " + icc.cardState);
        if (icc.cardState === 'ready') {
          icc.removeEventListener("cardstatechange", oncardstatechange);
          taskHelper.runNext();
        }
      });
    }
  });
});


taskHelper.runNext();
