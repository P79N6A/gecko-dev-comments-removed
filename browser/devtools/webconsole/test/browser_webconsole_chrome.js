









































const TEST_URI = "chrome://browser/content/browser.xul";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testChrome, false);
}

function testChrome() {
  browser.removeEventListener("DOMContentLoaded", testChrome, false);

  openConsole();

  let hud = HUDService.getHudByWindow(content);
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

  finishTest();
}

