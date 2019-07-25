








function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  let outputNode = hud.outputNode;
  let boxObject = outputNode.scrollBoxObject.element;

  for (let i = 0; i < 150; i++) {
    hud.console.log("test message " + i);
  }

  let oldScrollTop = boxObject.scrollTop;
  ok(oldScrollTop > 0, "scroll location is not at the top");

  hud.jsterm.execute("'hello world'");

  isnot(boxObject.scrollTop, oldScrollTop, "scroll location updated");

  oldScrollTop = boxObject.scrollTop;
  outputNode.scrollBoxObject.ensureElementIsVisible(outputNode.lastChild);

  is(boxObject.scrollTop, oldScrollTop, "scroll location is the same");

  finishTest();
}

function test() {
  addTab("data:text/html,Web Console test for bug 614793: jsterm result scroll");
  browser.addEventListener("load", tabLoad, true);
}

