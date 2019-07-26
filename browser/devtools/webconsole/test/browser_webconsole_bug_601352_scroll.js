









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

  HUD.jsterm.execute("1+1", performTest);

  function performTest(node) {
    let scrollNode = HUD.outputNode.parentNode;
    isnot(scrollNode.scrollTop, 0, "scroll location is not at the top");

    let rectNode = node.getBoundingClientRect();
    let rectOutput = scrollNode.getBoundingClientRect();

    
    let height = scrollNode.scrollHeight - scrollNode.scrollTop;

    
    let top = rectNode.top + scrollNode.scrollTop;
    let bottom = top + node.clientHeight;
    info("output height " + height + " node top " + top + " node bottom " + bottom + " node height " + node.clientHeight);

    ok(top >= 0 && bottom <= height, "last message is visible");

    finishTest();
  };
}

function test() {
  addTab("data:text/html;charset=utf-8,Web Console test for bug 601352");
  browser.addEventListener("load", function tabLoad(aEvent) {
    browser.removeEventListener(aEvent.type, tabLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

