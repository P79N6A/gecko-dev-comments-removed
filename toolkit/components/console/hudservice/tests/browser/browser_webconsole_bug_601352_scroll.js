









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
    isnot(HUD.outputNode.scrollTop, 0, "scroll location is not at the top");

    let node = HUD.outputNode.querySelector(".hud-group > *:last-child");
    let rectNode = node.getBoundingClientRect();
    let rectOutput = HUD.outputNode.getBoundingClientRect();

    
    let height = HUD.outputNode.scrollHeight - HUD.outputNode.scrollTop;

    
    let top = rectNode.top - rectOutput.top;

    
    let bottom = rectNode.bottom - rectOutput.top;

    ok(top >= 0 && bottom <= height, "last message is visible");

    finishTest();
  });
}

function test() {
  addTab("data:text/html,Web Console test for bug 601352");
  browser.addEventListener("load", tabLoad, true);
}

