











































const TEST_URI = "http://example.com/browser/toolkit/components/console/" +
                 "hudservice/tests/browser/" +
                 "test-bug-646025-console-file-location.html";

function test() {
  addTab("data:text/html,Web Console file location display test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  openConsole();
  hudId = HUDService.getHudIdByWindow(content);

  browser.addEventListener("load", testConsoleFileLocation, true);
  content.location = TEST_URI;
}

function testConsoleFileLocation(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  outputNode = HUDService.hudReferences[hudId].outputNode;

  executeSoon(function() {
    findLogEntry("test-file-location.js");
    findLogEntry("message for level");
    findLogEntry("test-file-location.js:5");
    findLogEntry("test-file-location.js:6");
    findLogEntry("test-file-location.js:7");
    findLogEntry("test-file-location.js:8");
    findLogEntry("test-file-location.js:9");

    finishTest();
  });
}

