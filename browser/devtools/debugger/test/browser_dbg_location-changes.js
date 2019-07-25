







var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;

function test()
{
  debug_tab_pane(STACK_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.contentWindow;

    testSimpleCall();
  });
}

function testSimpleCall() {
  gDebugger.DebuggerController.activeThread.addOneTimeListener("framesadded", function() {
    Services.tm.currentThread.dispatch({
      run: function() {
        var frames = gDebugger.DebuggerView.StackFrames._frames,
            childNodes = frames.childNodes;

        is(gDebugger.DebuggerController.activeThread.state, "paused",
          "Should only be getting stack frames while paused.");

        is(frames.querySelectorAll(".dbg-stackframe").length, 1,
          "Should have only one frame.");

        is(childNodes.length, frames.querySelectorAll(".dbg-stackframe").length,
          "All children should be frames.");

        testLocationChange();
      }
    }, 0);
  });

  gDebuggee.simpleCall();
}

function testLocationChange()
{
  gDebugger.DebuggerController.activeThread.resume(function() {
    gDebugger.DebuggerController.client.addOneTimeListener("tabNavigated", function(aEvent, aPacket) {
      ok(true, "tabNavigated event was fired.");
      gDebugger.DebuggerController.client.addOneTimeListener("tabAttached", function(aEvent, aPacket) {
        ok(true, "Successfully reattached to the tab again.");

        closeDebuggerAndFinish();
      });
    });
    content.location = TAB1_URL;
  });
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
});
