











const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testCloseButton, false);
}

function testCloseButton() {
  browser.removeEventListener("DOMContentLoaded", testCloseButton, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  hudBox = HUDService.getHeadsUpDisplay(hudId);

  HUDService.disableAnimation(hudId);
  executeSoon(function() {
    let closeButton = hudBox.querySelector(".jsterm-close-button");
    ok(closeButton != null, "we have the close button");

    

    EventUtils.synthesizeMouse(closeButton, 0, 0, {});

    executeSoon(function (){
      ok(!(hudId in HUDService.windowRegistry), "the console is closed when " +
         "the close button is pressed");
      closeButton = null;
      finishTest();
    });
  });
}
