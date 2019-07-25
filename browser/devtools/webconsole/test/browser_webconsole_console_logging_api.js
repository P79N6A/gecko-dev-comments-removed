









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);

  openConsole();

  hud = HUDService.getHudByWindow(content);
  hudId = hud.hudId;
  outputNode = hud.outputNode;

  testConsoleLoggingAPI("log");
  testConsoleLoggingAPI("info");
  testConsoleLoggingAPI("warn");
  testConsoleLoggingAPI("error");
  testConsoleLoggingAPI("debug"); 

  finishTest();
}

function testConsoleLoggingAPI(aMethod) {
  let console = content.wrappedJSObject.console;

  hud.jsterm.clearOutput();

  setStringFilter(hudId, "foo");
  console[aMethod]("foo-bar-baz");
  console[aMethod]("bar-baz");

  var nodes = outputNode.querySelectorAll(".hud-filtered-by-string");

  is(nodes.length, 1, "1 hidden " + aMethod  + " node found (via classList)");

  hud.jsterm.clearOutput();

  

  
  setStringFilter(hudId, "");
  HUDService.setFilterState(hudId, aMethod, false);
  console[aMethod]("foo-bar-baz");
  nodes = outputNode.querySelectorAll("description");

  is(nodes.length, 1,  aMethod + " logging turned off, 1 message hidden");

  hud.jsterm.clearOutput();
  HUDService.setFilterState(hudId, aMethod, true);
  console[aMethod]("foo-bar-baz");
  nodes = outputNode.querySelectorAll("description");

  is(nodes.length, 1, aMethod + " logging turned on, 1 message shown");

  hud.jsterm.clearOutput();
  setStringFilter(hudId, "");

  
  console[aMethod]("foo", "bar");

  let node = outputNode.querySelector(".hud-msg-node");
  ok(/foo bar/.test(node.textContent),
    "Emitted both console arguments");
}

function setStringFilter(aId, aValue) {
  hud.filterBox.value = aValue;
  HUDService.adjustVisibilityOnSearchStringChange(aId, aValue);
}

