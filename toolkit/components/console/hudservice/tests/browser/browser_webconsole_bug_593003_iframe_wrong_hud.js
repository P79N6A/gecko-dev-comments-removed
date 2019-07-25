




































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-bug-593003-iframe-wrong-hud.html";

const TEST_IFRAME_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-bug-593003-iframe-wrong-hud-iframe.html";

const TEST_DUMMY_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

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

  let hudId1 = HUDService.getHudIdByWindow(tab1.linkedBrowser.contentWindow);

  let display1 = HUDService.getOutputNodeById(hudId1);
  let outputNode1 = display1.querySelector(".hud-output-node");

  const successMsg1 = "Found the iframe network request in tab1";
  const errorMsg1 = "Failed to find the iframe network request in tab1";

  testLogEntry(outputNode1, TEST_IFRAME_URI,
               { success: successMsg1, err: errorMsg1}, true);

  let hudId2 = HUDService.getHudIdByWindow(tab2.linkedBrowser.contentWindow);
  let display2 = HUDService.getOutputNodeById(hudId2);
  let outputNode2 = display2.querySelector(".hud-output-node");

  isnot(display1, display2, "the two HUD displays must be different");
  isnot(outputNode1, outputNode2,
        "the two HUD outputNodes must be different");

  const successMsg2 = "The iframe network request is not in tab2";
  const errorMsg2 = "Found the iframe network request in tab2";

  testLogEntry(outputNode2, TEST_IFRAME_URI,
               { success: successMsg2, err: errorMsg2}, true, true);

  HUDService.deactivateHUDForContext(tab2);
  gBrowser.removeTab(tab2);

  tab1 = tab2 = null;

  finishTest();
}
