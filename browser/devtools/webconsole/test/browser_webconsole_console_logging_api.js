






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let testDriver = null;
let subtestDriver = null;

function test() {
  addTab(TEST_URI);

  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);

  openConsole(null, function(aHud) {
    hud = aHud;
    hudId = hud.hudId;
    outputNode = hud.outputNode;
    testDriver = testGen();
    testDriver.next();
  });
}

function testGen() {
  subtestGen("log");
  yield undefined;

  subtestGen("info");
  yield undefined;

  subtestGen("warn");
  yield undefined;

  subtestGen("error");
  yield undefined;

  subtestGen("debug"); 
  yield undefined;

  testDriver = subtestDriver = null;
  finishTest();

  yield undefined;
}

function subtestGen(aMethod) {
  subtestDriver = testConsoleLoggingAPI(aMethod);
  subtestDriver.next();
}

function testConsoleLoggingAPI(aMethod) {
  let console = content.wrappedJSObject.console;

  hud.jsterm.clearOutput();

  setStringFilter("foo");
  console[aMethod]("foo-bar-baz");
  console[aMethod]("bar-baz");

  function nextTest() {
    subtestDriver.next();
  }

  waitForSuccess({
    name: "1 hidden " + aMethod + " node via string filtering",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".hud-filtered-by-string").length == 1;
    },
    successFn: nextTest,
    failureFn: nextTest,
  });

  yield undefined;

  hud.jsterm.clearOutput();

  

  
  setStringFilter("");
  hud.setFilterState(aMethod, false);
  console[aMethod]("foo-bar-baz");

  waitForSuccess({
    name: "1 message hidden for " + aMethod + " (logging turned off)",
    validatorFn: function()
    {
      return outputNode.querySelectorAll("description").length == 1;
    },
    successFn: nextTest,
    failureFn: nextTest,
  });

  yield undefined;

  hud.jsterm.clearOutput();
  hud.setFilterState(aMethod, true);
  console[aMethod]("foo-bar-baz");

  waitForSuccess({
    name: "1 message shown for " + aMethod + " (logging turned on)",
    validatorFn: function()
    {
      return outputNode.querySelectorAll("description").length == 1;
    },
    successFn: nextTest,
    failureFn: nextTest,
  });

  yield undefined;

  hud.jsterm.clearOutput();
  setStringFilter("");

  
  console[aMethod]("foo", "bar");

  waitForSuccess({
    name: "show both console arguments for " + aMethod,
    validatorFn: function()
    {
      let node = outputNode.querySelector(".hud-msg-node");
      return node && /"foo" "bar"/.test(node.textContent);
    },
    successFn: nextTest,
    failureFn: nextTest,
  });

  yield undefined;
  testDriver.next();
  yield undefined;
}

function setStringFilter(aValue) {
  hud.ui.filterBox.value = aValue;
  hud.ui.adjustVisibilityOnSearchStringChange();
}

