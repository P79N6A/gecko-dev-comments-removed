







const TAB_URL = EXAMPLE_URL + "browser_dbg_script-switching.html";

var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;
var gSources = null;

function test()
{
  let scriptShown = false;
  let framesAdded = false;
  let testStarted = false;
  let resumed = false;

  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.panelWin;
    gSources = gDebugger.DebuggerView.Sources;
    resumed = true;

    gDebugger.addEventListener("Debugger:SourceShown", onScriptShown);

    gDebugger.DebuggerController.activeThread.addOneTimeListener("framesadded", function() {
      framesAdded = true;
      executeSoon(startTest);
    });

    executeSoon(function() {
      gDebuggee.firstCall();
    });
  });

  function onScriptShown(aEvent) {
    scriptShown = aEvent.detail.url.indexOf("-02.js") != -1;
    executeSoon(startTest);
  }

  function startTest()
  {
    if (scriptShown && framesAdded && resumed && !testStarted) {
      gDebugger.removeEventListener("Debugger:SourceShown", onScriptShown);
      testStarted = true;
      Services.tm.currentThread.dispatch({ run: testSelectLine }, 0);
    }
  }
}

function testSelectLine() {
  is(gDebugger.DebuggerController.activeThread.state, "paused",
    "Should only be getting stack frames while paused.");

  is(gSources.itemCount, 2, "Found the expected number of scripts.");

  ok(gDebugger.editor.getText().search(/debugger/) != -1,
    "The correct script was loaded initially.");

  
  
  executeSoon(function() {
    
    is(gDebugger.editor.getCaretPosition().line, 5,
       "The correct line is selected.");

    gDebugger.editor.addEventListener(SourceEditor.EVENTS.TEXT_CHANGED, function onChange() {
      
      if (gDebugger.editor.getText() == gDebugger.L10N.getStr("loadingText")) {
        return;
      }
      
      gDebugger.editor.removeEventListener(SourceEditor.EVENTS.TEXT_CHANGED, onChange);

      ok(gDebugger.editor.getText().search(/debugger/) == -1,
        "The second script is no longer displayed.");

      ok(gDebugger.editor.getText().search(/firstCall/) != -1,
        "The first script is displayed.");

      
      
      executeSoon(function(){
        
        is(gDebugger.editor.getCaretPosition().line, 4,
           "The correct line is selected.");

        closeDebuggerAndFinish();
      });
    });

    let frames = gDebugger.DebuggerView.StackFrames._container._list;
    let childNodes = frames.childNodes;

    is(frames.querySelectorAll(".dbg-stackframe").length, 4,
      "Should have two frames.");

    is(childNodes.length, frames.querySelectorAll(".dbg-stackframe").length,
      "All children should be frames.");

    EventUtils.sendMouseEvent({ type: "mousedown" },
      frames.querySelector("#stackframe-3"),
      gDebugger);
  });
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
  gSources = null;
});
