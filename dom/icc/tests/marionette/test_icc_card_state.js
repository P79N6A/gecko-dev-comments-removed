


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
      iccManager.addEventListener("iccdetected", function oniccdetected(evt) {
        log("icc iccdetected: " + evt.iccId);
        iccManager.removeEventListener("iccdetected", oniccdetected);
        taskHelper.runNext();
      });
    }
  });
});


taskHelper.runNext();
