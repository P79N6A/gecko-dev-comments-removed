









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);

  openConsole();

  testConsoleLoggingAPI("log");
  testConsoleLoggingAPI("info");
  testConsoleLoggingAPI("warn");
  testConsoleLoggingAPI("error");
  testConsoleLoggingAPI("debug"); 

  finishTest();
}

function testConsoleLoggingAPI(aMethod) {
  let hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let outputNode = hudBox.querySelector(".hud-output-node");

  HUDService.clearDisplay(hudId);

  setStringFilter(hudId, "foo");
  console[aMethod]("foo-bar-baz");
  console[aMethod]("bar-baz");

  var nodes = outputNode.querySelectorAll(".hud-filtered-by-string");

  is(nodes.length, 1, "1 hidden " + aMethod  + " node found (via classList)");

  HUDService.clearDisplay(hudId);

  

  
  setStringFilter(hudId, "");
  HUDService.setFilterState(hudId, aMethod, false);
  console[aMethod]("foo-bar-baz");
  nodes = outputNode.querySelectorAll("description");

  is(nodes.length, 1,  aMethod + " logging turned off, 1 message hidden");

  HUDService.clearDisplay(hudId);
  HUDService.setFilterState(hudId, aMethod, true);
  console[aMethod]("foo-bar-baz");
  nodes = outputNode.querySelectorAll("description");

  is(nodes.length, 1, aMethod + " logging turned on, 1 message shown");

  HUDService.clearDisplay(hudId);
  setStringFilter(hudId, "");

  
  console[aMethod]("foo", "bar");

  let node = outputNode.querySelector(".hud-msg-node");
  ok(/"foo" "bar"/.test(node.textContent),
    "Emitted both console arguments");
}

function setStringFilter(aId, aValue) {
  let hudBox = HUDService.getHeadsUpDisplay(aId);
  hudBox.querySelector(".hud-filter-box").value = aValue;
  HUDService.adjustVisibilityOnSearchStringChange(aId, aValue);
}

