










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testLogNodeClasses, false);
}

function testLogNodeClasses() {
  browser.removeEventListener("DOMContentLoaded", testLogNodeClasses,
                              false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;
  hudBox = HUDService.getHeadsUpDisplay(hudId);
  outputNode = hudBox.querySelector(".hud-output-node");

  ok(console, "console exists");
  console.log("I am a log message");
  console.error("I am an error");
  console.info("I am an info message");
  console.warn("I am a warning  message");

  let domLogEntries =
    outputNode.childNodes;

  let count = outputNode.childNodes.length;
  ok(count > 0, "LogCount: " + count);

  let klasses = ["hud-group",
                 "hud-msg-node hud-log",
                 "hud-msg-node hud-warn",
                 "hud-msg-node hud-info",
                 "hud-msg-node hud-error",
                 "hud-msg-node hud-exception",
                 "hud-msg-node hud-network"];

  function verifyClass(klass) {
    let len = klasses.length;
    for (var i = 0; i < len; i++) {
      if (klass == klasses[i]) {
        return true;
      }
    }
    return false;
  }

  for (var i = 0; i < count; i++) {
    let klass = domLogEntries[i].getAttribute("class");
    ok(verifyClass(klass),
       "Log Node class verified: " + klass);
  }

  finishTest();
}

