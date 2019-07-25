




































let stateBackup = ss.getBrowserState();

function test() {
  
  waitForExplicitFinish();

  let oldState = { windows: [{ tabs: [{ entries: [{ url: "example.com" }] }] }]};
  let pageData = {
    url: "about:sessionrestore",
    formdata: { "#sessionData": "(" + JSON.stringify(oldState) + ")" }
  };
  let state = { windows: [{ tabs: [{ entries: [pageData] }] }] };

  
  
  gBrowser.selectedTab.addEventListener("SSTabRestored", onSSTabRestored, true);

  ss.setBrowserState(JSON.stringify(state));
}

function onSSTabRestored(aEvent) {
  info("SSTabRestored event");
  gBrowser.selectedTab.removeEventListener("SSTabRestored", onSSTabRestored, true);
  gBrowser.selectedBrowser.addEventListener("input", onInput, true);
}

function onInput(aEvent) {
  info("input event");
  gBrowser.selectedBrowser.removeEventListener("input", onInput, true);

  
  
  let val = gBrowser.selectedBrowser.contentDocument.getElementById("sessionData").value;
  try {
    JSON.parse(val);
    ok(true, "JSON.parse succeeded");
  }
  catch (e) {
    ok(false, "JSON.parse failed");
  }
  cleanup();
}

function cleanup() {
  ss.setBrowserState(stateBackup);
  executeSoon(finish);
}
