











const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testCloseButton);
  }, true);
}

function testCloseButton(hud) {
  let hudId = hud.hudId;
  HUDService.disableAnimation(hudId);
  waitForFocus(function() {
    let closeButton = hud.ui.closeButton;
    ok(closeButton != null, "we have the close button");

    EventUtils.synthesizeMouse(closeButton, 2, 2, {}, hud.iframeWindow);

    ok(!(hudId in HUDService.hudReferences), "the console is closed when " +
       "the close button is pressed");

    finishTest();
  }, hud.iframeWindow);
}
