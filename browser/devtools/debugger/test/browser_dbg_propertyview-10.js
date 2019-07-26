







const TAB_URL = EXAMPLE_URL + "browser_dbg_with-frame.html";

var gPane = null;
var gTab = null;
var gDebugger = null;

function test()
{
  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gPane = aPane;
    gDebugger = gPane.contentWindow;

    testWithFrame();
  });
}

function testWithFrame()
{
  let count = 0;
  gDebugger.addEventListener("Debugger:FetchedVariables", function test() {
    
    
    if (++count <3) {
      info("Number of received Debugger:FetchedVariables events: " + count);
      return;
    }
    gDebugger.removeEventListener("Debugger:FetchedVariables", test, false);
    Services.tm.currentThread.dispatch({ run: function() {

      var frames = gDebugger.DebuggerView.StackFrames._frames,
          scopes = gDebugger.DebuggerView.Properties._vars,
          globalScope = scopes.lastChild,
          innerScope = scopes.firstChild,
          globalNodes = globalScope.querySelector(".details").childNodes,
          innerNodes = innerScope.querySelector(".details").childNodes;

      globalScope.expand();

      is(gDebugger.DebuggerController.activeThread.state, "paused",
        "Should only be getting stack frames while paused.");

      is(frames.querySelectorAll(".dbg-stackframe").length, 2,
        "Should have three frames.");

      is(scopes.children.length, 5, "Should have 5 variable scopes.");

      is(innerNodes[1].querySelector(".name").getAttribute("value"), "one",
        "Should have the right property name for |one|.");

      is(innerNodes[1].querySelector(".value").getAttribute("value"), "1",
        "Should have the right property value for |one|.");

      is(globalNodes[0].querySelector(".name").getAttribute("value"), "InstallTrigger",
        "Should have the right property name for |Array|.");

      is(globalNodes[0].querySelector(".value").getAttribute("value"), "undefined",
        "Should have the right property value for |InstallTrigger|.");

      let len = globalNodes.length - 1;
      is(globalNodes[len].querySelector(".name").getAttribute("value"), "window",
        "Should have the right property name for |window|.");

      is(globalNodes[len].querySelector(".value").getAttribute("value"), "[object Proxy]",
        "Should have the right property value for |window|.");

      resumeAndFinish();
    }}, 0);
  }, false);

  EventUtils.sendMouseEvent({ type: "click" },
    content.document.querySelector("button"),
    content.window);
}

function resumeAndFinish() {
  gDebugger.addEventListener("Debugger:AfterFramesCleared", function listener() {
    gDebugger.removeEventListener("Debugger:AfterFramesCleared", listener, true);
    Services.tm.currentThread.dispatch({ run: function() {
      var frames = gDebugger.DebuggerView.StackFrames._frames;

      is(frames.querySelectorAll(".dbg-stackframe").length, 0,
        "Should have no frames.");

      closeDebuggerAndFinish();
    }}, 0);
  }, true);

  gDebugger.DebuggerController.activeThread.resume();
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebugger = null;
});
