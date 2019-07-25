









































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
  testConsoleLoggingAPI("exception");

  finishTest();
}

function testConsoleLoggingAPI(aMethod) {
  let hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let outputNode = hudBox.querySelector(".hud-output-node");

  HUDService.clearDisplay(hudId);

  setStringFilter("foo");
  console[aMethod]("foo-bar-baz");
  console[aMethod]("bar-baz");
  var count = outputNode.querySelectorAll(".hud-filtered-by-string").length;
  ok(count == 1, "1 hidden " + aMethod  + " node found");
  HUDService.clearDisplay(hudId);

  
  
  setStringFilter("");
  HUDService.setFilterState(hudId, aMethod, false);
  console[aMethod]("foo-bar-baz");
  count = outputNode.querySelectorAll(".hud-filtered-by-type").length;
  is(count, 1, aMethod + " logging turned off, 1 message hidden");
  HUDService.clearDisplay(hudId);
  setStringFilter("");

  
  HUDService.clearDisplay(hudId);
  HUDService.setFilterState(hudId, aMethod, true);
  console[aMethod]("foo", "bar");

  let node = outputNode.querySelectorAll(".hud-msg-node")[0];
  ok(/foo bar/.test(node.textContent),
    "Emitted both console arguments");
}

function setStringFilter(aValue) {
  let hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  hudBox.querySelector(".hud-filter-box").value = aValue;
  HUDService.adjustVisibilityOnSearchStringChange(hudId, aValue);
}

