








































const TEST_URI = "chrome://browser/content/browser.xul";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testChrome, false);
}

function testChrome() {
  browser.removeEventListener("DOMContentLoaded", testChrome, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.hudReferences[hudId];
  ok(hud, "we have a console");
  
  hudBox = HUDService.getHeadsUpDisplay(hudId);
  ok(hudBox, "we have the console display");
  
  let jsterm = hud.jsterm;
  ok(jsterm, "we have a jsterm");

  let input = jsterm.inputNode;
  let outputNode = hudBox.querySelector(".jsterm-input-node");
  ok(outputNode, "we have an output node");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(jsterm.completeNode.value, "    ment", "'docu' completion");

  HUD = jsterm = input = null;
  finishTest();
}

