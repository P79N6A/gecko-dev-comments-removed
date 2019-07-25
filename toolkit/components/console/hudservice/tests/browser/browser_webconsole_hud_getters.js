










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testHUDGetters, false);
}

function testHUDGetters() {
  browser.removeEventListener("DOMContentLoaded", testHUDGetters, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  var HUD = HUDService.hudReferences[hudId];
  var jsterm = HUD.jsterm;
  var klass = jsterm.inputNode.getAttribute("class");
  ok(klass == "jsterm-input-node", "We have the input node.");

  var hudconsole = HUD.console;
  is(typeof hudconsole, "object", "HUD.console is an object");
  is(typeof hudconsole.log, "function", "HUD.console.log is a function");
  is(typeof hudconsole.info, "function", "HUD.console.info is a function");

  finishTest();
}

