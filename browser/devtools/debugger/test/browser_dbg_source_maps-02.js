






const TAB_URL = EXAMPLE_URL + "binary_search.html";

var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;
var gPrevPref = null;

function test()
{
  let scriptShown = false;
  let framesAdded = false;
  let resumed = false;
  let testStarted = false;

  gPrevPref = Services.prefs.getBoolPref(
    "devtools.debugger.source-maps-enabled");
  Services.prefs.setBoolPref("devtools.debugger.source-maps-enabled", true);

  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    resumed = true;
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.panelWin;

    gDebugger.addEventListener("Debugger:SourceShown", function _onSourceShown(aEvent) {
      gDebugger.removeEventListener("Debugger:SourceShown", _onSourceShown);
      
      is(gPrevPref, false,
        "The source maps functionality should be disabled by default.");
      is(gDebugger.Prefs.sourceMapsEnabled, true,
        "The source maps pref should be true from startup.");
      is(gDebugger.DebuggerView.Options._showOriginalSourceItem.getAttribute("checked"),
         "true", "Source maps should be enabled from startup. ")

      ok(aEvent.detail.url.indexOf(".coffee") != -1,
         "The debugger should show the source mapped coffee script file.");
      ok(aEvent.detail.url.indexOf(".js") == -1,
         "The debugger should not show the generated js script file.");
      ok(gDebugger.editor.getText().search(/isnt/) != -1,
         "The debugger's editor should have the coffee script source displayed.");
      ok(gDebugger.editor.getText().search(/function/) == -1,
         "The debugger's editor should not have the JS source displayed.");

      testToggleGeneratedSource();
    });
  });
}

function testToggleGeneratedSource() {
  gDebugger.addEventListener("Debugger:SourceShown", function _onSourceShown(aEvent) {
    gDebugger.removeEventListener("Debugger:SourceShown", _onSourceShown);

    is(gDebugger.Prefs.sourceMapsEnabled, false,
      "The source maps pref should have been set to false.");
    is(gDebugger.DebuggerView.Options._showOriginalSourceItem.getAttribute("checked"),
       "false", "Source maps should be enabled from startup. ")

    ok(aEvent.detail.url.indexOf(".coffee") == -1,
       "The debugger should not show the source mapped coffee script file.");
    ok(aEvent.detail.url.indexOf(".js") != -1,
       "The debugger should show the generated js script file.");
    ok(gDebugger.editor.getText().search(/isnt/) == -1,
       "The debugger's editor should have the coffee script source displayed.");
      ok(gDebugger.editor.getText().search(/function/) != -1,
         "The debugger's editor should not have the JS source displayed.");

    testSetBreakpoint();
  });

  
  gDebugger.DebuggerView.Options._showOriginalSourceItem.setAttribute("checked",
                                                                      "false");
  gDebugger.DebuggerView.Options._toggleShowOriginalSource();
}

function testSetBreakpoint() {
  let { activeThread } = gDebugger.DebuggerController;
  activeThread.setBreakpoint({
    url: EXAMPLE_URL + "binary_search.js",
    line: 7
  }, function (aResponse, bpClient) {
    ok(!aResponse.error,
       "Should be able to set a breakpoint in a JavaScript file.");
    testHitBreakpoint();
  });
}

function testHitBreakpoint() {
  let { activeThread } = gDebugger.DebuggerController;
  activeThread.resume(function (aResponse) {
    ok(!aResponse.error, "Shouldn't get an error resuming");
    is(aResponse.type, "resumed", "Type should be 'resumed'");

    activeThread.addOneTimeListener("framesadded", function (aEvent, aPacket) {
      
      let frames = gDebugger.DebuggerView.StackFrames._container._list;
      let childNodes = frames.childNodes;

      is(frames.querySelectorAll(".dbg-stackframe").length, 1,
        "Correct number of frames.");
      ok(frames.querySelector("#stackframe-0 .dbg-stackframe-details")
         .getAttribute("value").search(/js/),
         "First frame should be a JS frame.");

      waitForCaretPos(6, testToggleOnPause);
    });

    
    
    executeSoon(function() {
      gDebuggee.binary_search([0, 2, 3, 5, 7, 10], 5);
    });
  });
}

function testToggleOnPause() {
  gDebugger.addEventListener("Debugger:SourceShown", function _onSourceShown(aEvent) {
    gDebugger.removeEventListener("Debugger:SourceShown", _onSourceShown);

    is(gDebugger.Prefs.sourceMapsEnabled, true,
      "The source maps pref should have been set to true.");
    is(gDebugger.DebuggerView.Options._showOriginalSourceItem.getAttribute("checked"),
      "true", "Source maps should be enabled. ")

    ok(aEvent.detail.url.indexOf(".coffee") != -1,
       "The debugger should show the source mapped coffee script file.");
    ok(aEvent.detail.url.indexOf(".js") == -1,
       "The debugger should not show the generated js script file.");
    ok(gDebugger.editor.getText().search(/isnt/) != -1,
       "The debugger's editor should not have the coffee script source displayed.");
    ok(gDebugger.editor.getText().search(/function/) == -1,
       "The debugger's editor should have the JS source displayed.");

    
    let frames = gDebugger.DebuggerView.StackFrames._container._list;
    let childNodes = frames.childNodes;

    is(frames.querySelectorAll(".dbg-stackframe").length, 1,
      "Correct number of frames.");
    ok(frames.querySelector("#stackframe-0 .dbg-stackframe-details")
       .getAttribute("value").search(/coffee/),
       "First frame should be a coffee script frame.");

    waitForCaretPos(4, resumeAndFinish);
  });

  
  gDebugger.DebuggerView.Options._showOriginalSourceItem.setAttribute("checked",
                                                                      "true");
  gDebugger.DebuggerView.Options._toggleShowOriginalSource();
}

function resumeAndFinish()
{
  let { activeThread } = gDebugger.DebuggerController;
  activeThread.resume(function (aResponse) {
    ok(!aResponse.error, "Shouldn't get an error resuming");
    is(aResponse.type, "resumed", "Type should be 'resumed'");

    closeDebuggerAndFinish();
  });
}

function waitForCaretPos(number, callback)
{
  
  let count = 0;
  let intervalID = window.setInterval(function() {
    info("count: " + count + " ");
    if (++count > 50) {
      ok(false, "Timed out while polling for the line.");
      window.clearInterval(intervalID);
      return closeDebuggerAndFinish();
    }
    if (gDebugger.DebuggerView.editor.getCaretPosition().line != number) {
      return;
    }
    is(gDebugger.DebuggerView.editor.getCaretPosition().line, number,
       "The right line is focused.")
    
    window.clearInterval(intervalID);
    callback();
  }, 100);
}

registerCleanupFunction(function() {
  Services.prefs.setBoolPref("devtools.debugger.source-maps-enabled", false);
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
  gPrevPref = null;
});
