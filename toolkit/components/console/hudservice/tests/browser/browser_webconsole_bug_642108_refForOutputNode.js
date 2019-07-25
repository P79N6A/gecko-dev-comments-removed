









const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testHudRef,
                           false);
}

function testHudRef() {
  browser.removeEventListener("DOMContentLoaded",testHudRef, false);

  openConsole();
  let hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let hudRef = HUDService.getHudReferenceForOutputNode(hudBox);

  ok(hudRef, "We have a hudRef");

  let outBox = HUDService.getOutputNodeById(hudId);
  let hudRef2 = HUDService.getHudReferenceForOutputNode(outBox);

  ok(hudRef2, "We have the second hudRef");
  is(hudRef, hudRef2, "The two hudRefs are identical");

  finishTest();
}
