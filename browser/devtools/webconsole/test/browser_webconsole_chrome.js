









































const TEST_URI = "chrome://browser/content/browser.xul";

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("devtools.gcli.enable");
});

function test() {
  Services.prefs.setBoolPref("devtools.gcli.enable", false);
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

