






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let hud, outputNode;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    Task.spawn(runner);
  }, true);

  function* runner() {
    hud = yield openConsole();
    outputNode = hud.outputNode;

    let methods = ["log", "info", "warn", "error", "exception", "debug"];
    for (let method of methods) {
      yield testMethod(method);
    }

    executeSoon(finishTest);
  }
}

function* testMethod(aMethod) {
  let console = content.console;

  hud.jsterm.clearOutput();

  console[aMethod]("foo-bar-baz");
  console[aMethod]("baar-baz");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "foo-bar-baz",
    }, {
      text: "baar-baz",
    }],
  });

  setStringFilter("foo");

  is(outputNode.querySelectorAll(".filtered-by-string").length, 1,
     "1 hidden " + aMethod + " node via string filtering")

  hud.jsterm.clearOutput();

  
  

  console[aMethod]("foo-bar-baz");
  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "foo-bar-baz",
    }],
  });

  setStringFilter("");
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

  hud.setFilterState(filter, false);

  is(outputNode.querySelectorAll(".filtered-by-type").length, 1,
     "1 message hidden for " + aMethod + " (logging turned off)")

  hud.setFilterState(filter, true);

  is(outputNode.querySelectorAll(".message:not(.filtered-by-type)").length, 1,
     "1 message shown for " + aMethod + " (logging turned on)")

  hud.jsterm.clearOutput();

  
  console[aMethod]("foo", "bar");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: '"foo" "bar"',
      category: CATEGORY_WEBDEV,
    }],
  })
}

function setStringFilter(aValue) {
  hud.ui.filterBox.value = aValue;
  hud.ui.adjustVisibilityOnSearchStringChange();
}

