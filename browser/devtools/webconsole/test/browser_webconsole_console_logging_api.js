






"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();
  hud.jsterm.clearOutput();

  let outputNode = hud.outputNode;

  let methods = ["log", "info", "warn", "error", "exception", "debug"];
  for (let method of methods) {
    yield testMethod(method, hud, outputNode);
  }
});

function* testMethod(aMethod, aHud, aOutputNode) {
  let console = content.console;

  aHud.jsterm.clearOutput();

  console[aMethod]("foo-bar-baz");
  console[aMethod]("baar-baz");

  yield waitForMessages({
    webconsole: aHud,
    messages: [{
      text: "foo-bar-baz",
    }, {
      text: "baar-baz",
    }],
  });

  setStringFilter("foo", aHud);

  is(aOutputNode.querySelectorAll(".filtered-by-string").length, 1,
     "1 hidden " + aMethod + " node via string filtering")

  aHud.jsterm.clearOutput();

  
  

  console[aMethod]("foo-bar-baz");
  yield waitForMessages({
    webconsole: aHud,
    messages: [{
      text: "foo-bar-baz",
    }],
  });

  setStringFilter("", aHud);
  let filter;
  switch(aMethod) {
    case "debug":
      filter = "log";
      break;
    case "exception":
      filter = "error";
      break;
    default:
      filter = aMethod;
      break;
  }

  aHud.setFilterState(filter, false);

  is(aOutputNode.querySelectorAll(".filtered-by-type").length, 1,
     "1 message hidden for " + aMethod + " (logging turned off)")

  aHud.setFilterState(filter, true);

  is(aOutputNode.querySelectorAll(".message:not(.filtered-by-type)").length, 1,
     "1 message shown for " + aMethod + " (logging turned on)")

  aHud.jsterm.clearOutput();

  
  console[aMethod]("foo", "bar");

  yield waitForMessages({
    webconsole: aHud,
    messages: [{
      text: '"foo" "bar"',
      category: CATEGORY_WEBDEV,
    }],
  })
}

function setStringFilter(aValue, aHud) {
  aHud.ui.filterBox.value = aValue;
  aHud.ui.adjustVisibilityOnSearchStringChange();
}
