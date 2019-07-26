







var gClient = null;
var gTab = null;
var gHomeTab = null;
var gThreadClient = null;
var gNewGlobal = false;
var gAttached = false;
var gChromeScript = false;

const DEBUGGER_TAB_URL = EXAMPLE_URL + "browser_dbg_debuggerstatement.html";

function test()
{
  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.connect(function(aType, aTraits) {
    gTab = addTab(DEBUGGER_TAB_URL, function() {
      gClient.listTabs(function(aResponse) {
        let dbg = aResponse.chromeDebugger;
        ok(dbg, "Found a chrome debugging actor.");

        gClient.addOneTimeListener("newGlobal", function() gNewGlobal = true);
        gClient.addListener("newScript", onNewScript);

        gClient.attachThread(dbg, function(aResponse, aThreadClient) {
          gThreadClient = aThreadClient;
          ok(!aResponse.error, "Attached to the chrome debugger.");
          gAttached = true;

          
          gHomeTab = gBrowser.addTab("about:home");

          finish_test();
        });
      });
    });
  });
}

function onNewScript(aEvent, aScript)
{
  if (aScript.url.startsWith("chrome:")) {
    gChromeScript = true;
  }
  finish_test();
}

function finish_test()
{
  if (!gAttached || !gChromeScript) {
    return;
  }
  gClient.removeListener("newScript", onNewScript);
  gThreadClient.resume(function(aResponse) {
    removeTab(gHomeTab);
    removeTab(gTab);
    gClient.close(function() {
      ok(gNewGlobal, "Received newGlobal event.");
      ok(gChromeScript, "Received newScript event for a chrome: script.");
      finish();
    });
  });
}
