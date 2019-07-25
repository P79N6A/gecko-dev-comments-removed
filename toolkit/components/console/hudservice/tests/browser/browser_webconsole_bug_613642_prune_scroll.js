








function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  let outputNode = hud.outputNode;
  let oldPref = Services.prefs.getIntPref("devtools.hud.loglimit.console");

  Services.prefs.setIntPref("devtools.hud.loglimit.console", 140);
  let scrollBoxElement = outputNode.scrollBoxObject.element;
  let boxObject = outputNode.scrollBoxObject;

  for (let i = 0; i < 150; i++) {
    hud.console.log("test message " + i);
  }

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

  
  hud.console.log("hello world");

  
  
  isnot(scrollBoxElement.scrollTop, oldScrollTop,
        "scroll location updated (added a message)");

  isnot(outputNode.firstChild, firstNode,
        "first message removed");

  Services.prefs.setIntPref("devtools.hud.loglimit.console", oldPref);
  finishTest();
}

function test() {
  addTab("data:text/html,Web Console test for bug 613642: maintain scroll with pruning of old messages");
  browser.addEventListener("load", tabLoad, true);
}
