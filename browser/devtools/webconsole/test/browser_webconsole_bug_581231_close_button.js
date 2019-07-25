











const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testCloseButton, false);
}

function testCloseButton() {
  browser.removeEventListener("DOMContentLoaded", testCloseButton, false);

  openConsole();

  let hud = HUDService.getHudByWindow(content);
  let hudId = hud.hudId;

  HUDService.disableAnimation(hudId);
  executeSoon(function() {
    let closeButton = hud.HUDBox.querySelector(".webconsole-close-button");
    ok(closeButton != null, "we have the close button");

    

    closeButton.addEventListener("command", function() {
      closeButton.removeEventListener("command", arguments.callee, false);

      ok(!(hudId in HUDService.hudReferences), "the console is closed when " +
         "the close button is pressed");
      closeButton = null;
      finishTest();
    }, false);

    EventUtils.synthesizeMouse(closeButton, 2, 2, {});
  });
}
