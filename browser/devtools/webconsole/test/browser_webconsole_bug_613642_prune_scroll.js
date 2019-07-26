








let hud, testDriver;

function testNext() {
  testDriver.next();
}

function testGen() {
  hud.jsterm.clearOutput();

  let outputNode = hud.outputNode;
  let oldPref = Services.prefs.getIntPref("devtools.hud.loglimit.console");

  Services.prefs.setIntPref("devtools.hud.loglimit.console", 140);
  let scrollBoxElement = outputNode.scrollBoxObject.element;
  let boxObject = outputNode.scrollBoxObject;

  for (let i = 0; i < 150; i++) {
    content.console.log("test message " + i);
  }

  waitForSuccess({
    name: "150 console.log messages displayed",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".hud-log").length == 140;
    },
    successFn: testNext,
    failureFn: finishTest,
  });

  yield undefined;

  let oldScrollTop = scrollBoxElement.scrollTop;
  ok(oldScrollTop > 0, "scroll location is not at the top");

  let firstNode = outputNode.firstChild;
  ok(firstNode, "found the first message");

  let msgNode = outputNode.querySelectorAll("richlistitem")[80];
  ok(msgNode, "found the 80th message");

  
  boxObject.ensureElementIsVisible(msgNode);

  isnot(scrollBoxElement.scrollTop, oldScrollTop,
        "scroll location updated (scrolled to message)");

  oldScrollTop = scrollBoxElement.scrollTop;

  
  content.console.log("hello world");

  waitForSuccess({
    name: "console.log message #151 displayed",
    validatorFn: function()
    {
      return outputNode.textContent.indexOf("hello world") > -1;
    },
    successFn: testNext,
    failureFn: finishTest,
  });

  yield undefined;

  
  
  isnot(scrollBoxElement.scrollTop, oldScrollTop,
        "scroll location updated (added a message)");

  isnot(outputNode.firstChild, firstNode,
        "first message removed");

  Services.prefs.setIntPref("devtools.hud.loglimit.console", oldPref);

  hud = testDriver = null;
  finishTest();

  yield undefined;
}

function test() {
  addTab("data:text/html;charset=utf-8,Web Console test for bug 613642: maintain scroll with pruning of old messages");
  browser.addEventListener("load", function tabLoad(aEvent) {
    browser.removeEventListener(aEvent.type, tabLoad, true);

    openConsole(null, function(aHud) {
      hud = aHud;
      testDriver = testGen();
      testDriver.next();
    });
  }, true);
}
