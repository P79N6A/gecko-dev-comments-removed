


const STATE_CHANGED_TOPIC = "fxa-migration:state-changed";

let imports = {};
Cu.import("resource://services-sync/FxaMigrator.jsm", imports);

add_task(function* test() {
  
  let buttonPromise = promiseButtonMutation();
  Services.obs.notifyObservers(null, "fxa-migration:state-changed",
                               fxaMigrator.STATE_USER_FXA);
  let buttonState = yield buttonPromise;
  assertButtonState(buttonState, "migrate-signup", true);

  
  buttonPromise = promiseButtonMutation();
  let email = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
  email.data = "foo@example.com";
  Services.obs.notifyObservers(email, "fxa-migration:state-changed",
                               fxaMigrator.STATE_USER_FXA_VERIFIED);
  buttonState = yield buttonPromise;
  assertButtonState(buttonState, "migrate-verify", true,
                    "foo@example.com not verified");

  
  buttonPromise = promiseButtonMutation();
  Services.obs.notifyObservers(null, "fxa-migration:state-changed", null);
  buttonState = yield buttonPromise;
  
  
  
  assertButtonState(buttonState, "", true);
});

function assertButtonState(buttonState, expectedStatus, expectedVisible,
                           expectedLabel=undefined) {
  Assert.equal(buttonState.fxastatus, expectedStatus,
               "Button fxstatus attribute");
  Assert.equal(!buttonState.hidden, expectedVisible, "Button visibility");
  if (expectedLabel !== undefined) {
    Assert.equal(buttonState.label, expectedLabel, "Button label");
  }
}

function promiseButtonMutation() {
  return new Promise((resolve, reject) => {
    let obs = new MutationObserver(mutations => {
      info("Observed mutations for attributes: " +
           mutations.map(m => m.attributeName));
      if (mutations.some(m => m.attributeName == "fxastatus")) {
        obs.disconnect();
        resolve({
          fxastatus: gFxAccounts.button.getAttribute("fxastatus"),
          hidden: gFxAccounts.button.hidden,
          label: gFxAccounts.button.label,
        });
      }
    });
    obs.observe(gFxAccounts.button, { attributes: true });
  });
}
