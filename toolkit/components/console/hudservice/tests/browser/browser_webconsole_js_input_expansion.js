









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testJSInputExpansion, false);
}

function testJSInputExpansion() {
  browser.removeEventListener("DOMContentLoaded", testJSInputExpansion,
                              false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  let HUD = HUDService.hudReferences[hudId];
  let jsterm = HUD.jsterm;
  let input = jsterm.inputNode;
  input.focus();

  is(input.getAttribute("multiline"), "true", "multiline is enabled");
  
  input.value = "hello\nworld\n";
  let length = input.value.length;
  input.selectionEnd = length;
  input.selectionStart = length;
  function getHeight()
  {
    let h = browser.contentDocument.defaultView.getComputedStyle(input, null)
      .getPropertyValue("height");
    return parseInt(h);
  }
  let initialHeight = getHeight();
  
  
  EventUtils.synthesizeKey("d", {});
  let newHeight = getHeight();
  ok(initialHeight < newHeight, "Height changed: " + newHeight);

  
  input.value = "row1\nrow2\nrow3\nrow4\nrow5\nrow6\nrow7\nrow8\nrow9\nrow10\n";
  length = input.value.length;
  input.selectionEnd = length;
  input.selectionStart = length;
  EventUtils.synthesizeKey("d", {});
  let newerHeight = getHeight();

  ok(newerHeight > newHeight, "height changed: " + newerHeight);

  
  input.value = "";
  EventUtils.synthesizeKey("d", {});
  let height = getHeight();
  info("height: " + height);
  info("initialHeight: " + initialHeight);
  let finalHeightDifference = Math.abs(initialHeight - height);
  ok(finalHeightDifference <= 1, "height shrank to original size within 1px");

  finishTest();
}
