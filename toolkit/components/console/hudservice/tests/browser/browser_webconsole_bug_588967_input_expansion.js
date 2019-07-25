





































const Cu = Components.utils;

Cu.import("resource:///modules/HUDService.jsm");

const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  waitForExplicitFinish();
  content.location.href = TEST_URI;
  waitForFocus(onFocus);
}

function onFocus() {
  gBrowser.selectedBrowser.addEventListener("DOMContentLoaded",
                                            testInputExpansion, false);
}

function testInputExpansion() {
  gBrowser.selectedBrowser.removeEventListener("DOMContentLoaded",
                                               testInputExpansion, false);

  HUDService.activateHUDForContext(gBrowser.selectedTab);

  let hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let input = hudBox.querySelector(".jsterm-input-node");

  input.focus();

  is(input.getAttribute("multiline"), "true", "multiline is enabled");

  let ordinaryHeight = input.clientHeight;

  
  input.value = "hello\nworld\n";
  let length = input.value.length;
  input.selectionEnd = length;
  input.selectionStart = length;
  
  
  EventUtils.synthesizeKey("d", {});
  ok(input.clientHeight > ordinaryHeight, "the input expanded");

  
  input.value = "";
  EventUtils.synthesizeKey("d", {});
  is(input.clientHeight, ordinaryHeight, "the input's height is normal again");

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  finish();
}

