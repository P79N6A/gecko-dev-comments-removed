








const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-621644-jsterm-dollar.html";

function test$(HUD) {
  HUD.jsterm.clearOutput();

  HUD.jsterm.execute("$(document.body)", (msg) => {
    ok(msg.textContent.indexOf("<p>") > -1,
       "jsterm output is correct for $()");

    test$$(HUD);
  });
}

function test$$(HUD) {
  HUD.jsterm.clearOutput();

  HUD.jsterm.setInputValue();
  HUD.jsterm.execute("$$(document)", (msg) => {
    ok(msg.textContent.indexOf("621644") > -1,
       "jsterm output is correct for $$()");
    finishTest();
  });
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, test$);
  }, true);
}
