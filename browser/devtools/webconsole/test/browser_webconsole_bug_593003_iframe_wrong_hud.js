





































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-bug-593003-iframe-wrong-hud.html";

const TEST_IFRAME_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-bug-593003-iframe-wrong-hud-iframe.html";

const TEST_DUMMY_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

let tab1, tab2;

function test() {
  addTab(TEST_URI);
  tab1 = tab;
  browser.addEventListener("load", tab1Loaded, true);
}













function tab1Loaded(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  browser.contentWindow.wrappedJSObject.console.log("FOO");
  try {
    openConsole();
  }
  catch (ex) {
    log(ex);
    log(ex.stack);
  }

  tab2 = gBrowser.addTab(TEST_DUMMY_URI);
  gBrowser.selectedTab = tab2;
  gBrowser.selectedBrowser.addEventListener("load", tab2Loaded, true);
}

function tab2Loaded(aEvent) {
  tab2.linkedBrowser.removeEventListener(aEvent.type, arguments.callee, true);

  HUDService.activateHUDForContext(gBrowser.selectedTab);

  tab1.linkedBrowser.addEventListener("load", tab1Reloaded, true);
  tab1.linkedBrowser.contentWindow.location.reload();
}

function tab1Reloaded(aEvent) {
  tab1.linkedBrowser.removeEventListener(aEvent.type, arguments.callee, true);

  let hud1 = HUDService.getHudByWindow(tab1.linkedBrowser.contentWindow);
  let outputNode1 = hud1.outputNode;

  let msg = "Found the iframe network request in tab1";
  testLogEntry(outputNode1, TEST_IFRAME_URI, msg, true);

  let hud2 = HUDService.getHudByWindow(tab2.linkedBrowser.contentWindow);
  let outputNode2 = hud2.outputNode;

  isnot(outputNode1, outputNode2,
        "the two HUD outputNodes must be different");

  msg = "Didn't find the iframe network request in tab2";
  testLogEntry(outputNode2, TEST_IFRAME_URI, msg, true, true);

  HUDService.deactivateHUDForContext(tab2);
  gBrowser.removeTab(tab2);

  finishTest();
}
