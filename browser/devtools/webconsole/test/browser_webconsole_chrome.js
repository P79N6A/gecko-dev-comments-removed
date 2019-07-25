









































function test() {
  addTab(getBrowserURL());
  browser.addEventListener("DOMContentLoaded", function onLoad() {
    browser.removeEventListener("DOMContentLoaded", onLoad, true);
    openConsole();
    testChrome(HUDService.getHudByWindow(content));
  }, true);
}

function testChrome(hud) {
  ok(hud, "we have a console");
  
  ok(hud.HUDBox, "we have the console display");

  let jsterm = hud.jsterm;
  ok(jsterm, "we have a jsterm");

  let input = jsterm.inputNode;
  ok(hud.outputNode, "we have an output node");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(jsterm.completeNode.value, "    ment", "'docu' completion");

  gBrowser.removeCurrentTab();
  executeSoon(finishTest);
}

