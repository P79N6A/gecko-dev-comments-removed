









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-repeated-messages.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud) {
  
  waitForSuccess({
    name: "css warnings displayed",
    validatorFn: function()
    {
      return hud.outputNode.querySelectorAll(".webconsole-msg-cssparser")
             .length == 2;
    },
    successFn: testCSSRepeats.bind(null, hud),
    failureFn: finishTest,
  });
}

function repeatCountForNode(aNode) {
  return aNode.querySelector(".webconsole-msg-repeat").getAttribute("value");
}

function testCSSRepeats(hud) {
  let msgs = hud.outputNode.querySelectorAll(".webconsole-msg-cssparser");
  is(repeatCountForNode(msgs[0]), 1, "no repeats for the first css warning");
  is(repeatCountForNode(msgs[1]), 1, "no repeats for the second css warning");

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    testAfterReload(hud);
  }, true);
  content.location.reload();
}

function testAfterReload(hud) {
  waitForSuccess({
    name: "message repeats increased",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-msg-repeat")
             .getAttribute("value") == 2;
    },
    successFn: testCSSRepeatsAfterReload.bind(null, hud),
    failureFn: finishTest,
  });
}

function testCSSRepeatsAfterReload(hud) {
  let msgs = hud.outputNode.querySelectorAll(".webconsole-msg-cssparser");
  is(msgs.length, 2, "two css warnings after reload");
  is(repeatCountForNode(msgs[0]), 2, "two repeats for the first css warning");
  is(repeatCountForNode(msgs[1]), 2, "two repeats for the second css warning");

  hud.jsterm.clearOutput();
  content.wrappedJSObject.testConsole();

  waitForSuccess({
    name: "console API messages displayed",
    validatorFn: function()
    {
      return hud.outputNode.querySelectorAll(".webconsole-msg-console")
             .length == 3;
    },
    successFn: testConsoleRepeats.bind(null, hud),
    failureFn: finishTest,
  });
}

function testConsoleRepeats(hud) {
  let msgs = hud.outputNode.querySelectorAll(".webconsole-msg-console");
  is(repeatCountForNode(msgs[0]), 2, "repeats for the first console message");
  is(repeatCountForNode(msgs[1]), 1,
     "no repeats for the second console log message");
  is(repeatCountForNode(msgs[2]), 1, "no repeats for the console.error message");

  hud.jsterm.clearOutput();
  hud.jsterm.execute("undefined");
  content.console.log("undefined");

  waitForSuccess({
    name: "messages displayed",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-msg-console");
    },
    successFn: function() {
      is(hud.outputNode.childNodes.length, 3,
         "correct number of messages displayed");
      executeSoon(finishTest);
    },
    failureFn: finishTest,
  });
}
