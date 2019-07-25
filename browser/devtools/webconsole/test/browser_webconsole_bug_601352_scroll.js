









function consoleOpened(HUD) {
  HUD.jsterm.clearOutput();

  let longMessage = "";
  for (let i = 0; i < 50; i++) {
    longMessage += "LongNonwrappingMessage";
  }

  for (let i = 0; i < 50; i++) {
    content.console.log("test message " + i);
  }

  content.console.log(longMessage);

  for (let i = 0; i < 50; i++) {
    content.console.log("test message " + i);
  }

  HUD.jsterm.execute("1+1");

  function performTest() {
    let scrollBox = HUD.outputNode.scrollBoxObject.element;
    isnot(scrollBox.scrollTop, 0, "scroll location is not at the top");

    let node = HUD.outputNode.getItemAtIndex(HUD.outputNode.itemCount - 1);
    let rectNode = node.getBoundingClientRect();
    let rectOutput = HUD.outputNode.getBoundingClientRect();

    
    let height = scrollBox.scrollHeight - scrollBox.scrollTop;

    
    let top = rectNode.top - rectOutput.top;

    
    let bottom = rectNode.bottom - rectOutput.top;

    ok(top >= 0 && Math.floor(bottom) <= height + 1,
       "last message is visible");

    finishTest();
  };

  waitForSuccess({
    name: "console output displayed",
    validatorFn: function()
    {
      return HUD.outputNode.itemCount >= 103;
    },
    successFn: performTest,
    failureFn: function() {
      info("itemCount: " + HUD.outputNode.itemCount);
      finishTest();
    },
  });
}

function test() {
  addTab("data:text/html;charset=utf-8,Web Console test for bug 601352");
  browser.addEventListener("load", function tabLoad(aEvent) {
    browser.removeEventListener(aEvent.type, tabLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

