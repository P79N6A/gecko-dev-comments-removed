








const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-bug-621644-jsterm-dollar.html";

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  waitForFocus(function () {
    openConsole();

    let hudId = HUDService.getHudIdByWindow(content);
    let HUD = HUDService.hudReferences[hudId];

    HUD.jsterm.clearOutput();

    HUD.jsterm.setInputValue("$(document.body)");
    HUD.jsterm.execute();

    let outputItem = HUD.outputNode.
                     querySelector(".webconsole-msg-output:last-child");
    ok(outputItem.textContent.indexOf("<p>") > -1,
       "jsterm output is correct for $()");

    HUD.jsterm.clearOutput();

    HUD.jsterm.setInputValue("$$(document)");
    HUD.jsterm.execute();

    outputItem = HUD.outputNode.
                     querySelector(".webconsole-msg-output:last-child");
    ok(outputItem.textContent.indexOf("621644") > -1,
       "jsterm output is correct for $$()");

    executeSoon(finishTest);
  }, content);
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad, true);
}
