









function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  let HUD = HUDService.hudReferences[hudId];

  let longMessage = "";
  for (let i = 0; i < 50; i++) {
    longMessage += "LongNonwrappingMessage";
  }

  for (let i = 0; i < 50; i++) {
    HUD.console.log("test message " + i);
  }

  HUD.console.log(longMessage);

  for (let i = 0; i < 50; i++) {
    HUD.console.log("test message " + i);
  }

  HUD.jsterm.execute("1+1");

  executeSoon(function() {
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
  });
}

function test() {
  addTab("data:text/html,Web Console test for bug 601352");
  browser.addEventListener("load", tabLoad, true);
}

