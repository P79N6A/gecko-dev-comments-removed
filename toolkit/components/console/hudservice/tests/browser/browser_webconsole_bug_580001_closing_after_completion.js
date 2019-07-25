










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testClosingAfterCompletion,
                           false);
}

function testClosingAfterCompletion() {
  browser.removeEventListener("DOMContentLoaded",
                              testClosingAfterCompletion, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  hudBox = HUDService.getHeadsUpDisplay(hudId);
  let inputNode = hudBox.querySelector(".jsterm-input-node");

  let errorWhileClosing = false;
  function errorListener(evt) {
    browser.removeEventListener("error", errorListener, false);
    errorWhileClosing = true;
  }

  browser.addEventListener("error", errorListener, false);

  
  inputNode.focus();
  EventUtils.synthesizeKey("k", { accelKey: true, shiftKey: true });

  
  
  executeSoon(function() {
    is(errorWhileClosing, false, "no error while closing the WebConsole");
    finishTest();
  });
}

