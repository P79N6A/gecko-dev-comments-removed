





var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;

function test() {
  debug_tab_pane(STACK_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.debuggerWindow;

    testSimpleCall();
  });
}

function testSimpleCall() {
  gPane.activeThread.addOneTimeListener("framesadded", function() {
    Services.tm.currentThread.dispatch({ run: function() {

      let frames = gDebugger.DebuggerView.Stackframes._frames;
      let childNodes = frames.childNodes;

      is(gDebugger.StackFrames.activeThread.state, "paused",
        "Should only be getting stack frames while paused.");

      is(frames.querySelectorAll(".dbg-stackframe").length, 1,
        "Should have only one frame.");

      is(childNodes.length, frames.querySelectorAll(".dbg-stackframe").length,
        "All children should be frames.");

      resumeAndFinish();
    }}, 0);
  });

  gDebuggee.simpleCall();
}

function resumeAndFinish() {
  gDebugger.StackFrames.activeThread.resume(function() {
    removeTab(gTab);
    finish();
  });
}
