








































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
  is(input.value, "document", "'docu' completion");
  is(input.selectionStart, 4, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");

  HUD = jsterm = input = null;
  finishTest();
}

