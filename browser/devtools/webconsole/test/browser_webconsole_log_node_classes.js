










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(aHud) {
  let console = content.console;
  outputNode = aHud.outputNode;

  ok(console, "console exists");
  console.log("I am a log message");
  console.error("I am an error");
  console.info("I am an info message");
  console.warn("I am a warning  message");

  waitForSuccess({
    name: "console.warn displayed",
    validatorFn: function()
    {
      return aHud.outputNode.textContent.indexOf("a warning") > -1;
    },
    successFn: testLogNodeClasses,
    failureFn: finishTest,
  });
}

function testLogNodeClasses() {
  let domLogEntries = outputNode.childNodes;

  let count = outputNode.childNodes.length;
  ok(count > 0, "LogCount: " + count);

  let klasses = ["hud-log",
                 "hud-warn",
                 "hud-info",
                 "hud-error",
                 "hud-exception",
                 "hud-network"];

  function verifyClass(classList) {
    let len = klasses.length;
    for (var i = 0; i < len; i++) {
      if (classList.contains(klasses[i])) {
        return true;
      }
    }
    return false;
  }

  for (var i = 0; i < count; i++) {
    let classList = domLogEntries[i].classList;
    ok(verifyClass(classList),
       "Log Node class verified: " + domLogEntries[i].getAttribute("class"));
  }

  finishTest();
}

