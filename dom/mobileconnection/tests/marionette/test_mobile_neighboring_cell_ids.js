


MARIONETTE_TIMEOUT = 30000;
MARIONETTE_HEAD_JS = "head_chrome.js";


startTestBase(function() {
  
  
  return getNeighboringCellIds()
    .then(() => {
      ok(false, "should not success");
    }, (aErrorMsg) => {
      is(aErrorMsg, "RequestNotSupported",
         "Failed to getNeighboringCellIds: " + aErrorMsg);
    });
});
