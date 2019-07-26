








const TAB_URL = EXAMPLE_URL + "browser_dbg_script-switching.html";
var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;
var gSources = null;

function test()
{
  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.panelWin;

    testInitialLoad();
  });
}

function testInitialLoad() {
  gDebugger.DebuggerController.activeThread.addOneTimeListener("framesadded", function() {
    executeSoon(function() {
      validateFirstPage();
      testLocationChange();
    });
  });

  gDebuggee.firstCall();
}

function testLocationChange()
{
  gDebugger.DebuggerController.activeThread.resume(function() {
    gDebugger.DebuggerController._target.once("navigate", function onTabNavigated(aEvent, aPacket) {
      ok(true, "tabNavigated event was fired.");
      info("Still attached to the tab.");

      gDebugger.addEventListener("Debugger:AfterSourcesAdded", function _onEvent(aEvent) {
        gDebugger.removeEventListener(aEvent.type, _onEvent);

        executeSoon(function() {
          validateSecondPage();
          testBack();
        });
      });
    });
    gDebugger.DebuggerController.client.activeTab.navigateTo(STACK_URL);
  });
}

function testBack()
{
  gDebugger.DebuggerController._target.once("navigate", function onTabNavigated(aEvent, aPacket) {
    ok(true, "tabNavigated event was fired after going back.");
    info("Still attached to the tab.");

    gDebugger.addEventListener("Debugger:AfterSourcesAdded", function _onEvent(aEvent) {
      gDebugger.removeEventListener(aEvent.type, _onEvent);

      executeSoon(function() {
        validateFirstPage();
        testForward();
      });
    });
  });

  info("Going back.");
  content.history.back();
}

function testForward()
{
  gDebugger.DebuggerController._target.once("navigate", function onTabNavigated(aEvent, aPacket) {
    ok(true, "tabNavigated event was fired after going forward.");
    info("Still attached to the tab.");

    gDebugger.addEventListener("Debugger:AfterSourcesAdded", function _onEvent(aEvent) {
      gDebugger.removeEventListener(aEvent.type, _onEvent);

      executeSoon(function() {
        validateSecondPage();
        closeDebuggerAndFinish();
      });
    });
  });

  info("Going forward.");
  content.history.forward();
}

function validateFirstPage() {
  gSources = gDebugger.DebuggerView.Sources;

  is(gSources.itemCount, 2,
    "Found the expected number of scripts.");

  ok(gDebugger.DebuggerView.Sources.containsLabel("test-script-switching-01.js"),
     "Found the first script label.");
  ok(gDebugger.DebuggerView.Sources.containsLabel("test-script-switching-02.js"),
     "Found the second script label.");
}

function validateSecondPage() {
  gSources = gDebugger.DebuggerView.Sources;

  is(gSources.itemCount, 1,
    "Found the expected number of scripts.");

  ok(gDebugger.DebuggerView.Sources.containsLabel("browser_dbg_stack.html"),
     "Found the single script label.");
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
  gSources = null;
});
