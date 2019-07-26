







const TAB_URL = EXAMPLE_URL + "browser_dbg_pause-exceptions.html";

var gPane = null;
var gTab = null;
var gDebugger = null;
var gCount = 0;

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
  gPane.contentWindow.gClient.addOneTimeListener("paused", function() {
    gDebugger.addEventListener("Debugger:FetchedVariables", function testA() {
      
      
      if (++gCount < 2) {
        is(gCount, 1, "A. First Debugger:FetchedVariables event received.");
        return;
      }
      is(gCount, 2, "A. Second Debugger:FetchedVariables event received.");
      gDebugger.removeEventListener("Debugger:FetchedVariables", testA, false);

      is(gDebugger.DebuggerController.activeThread.state, "paused",
        "Should be paused now.");

      gDebugger.DebuggerView.Options._pauseOnExceptionsItem.setAttribute("checked", "true");
      gDebugger.DebuggerView.Options._togglePauseOnExceptions();

      gCount = 0;
      gPane.contentWindow.gClient.addOneTimeListener("resumed", function() {
        gDebugger.addEventListener("Debugger:FetchedVariables", function testB() {
          
          
          if (++gCount < 2) {
            is(gCount, 1, "B. First Debugger:FetchedVariables event received.");
            return;
          }
          is(gCount, 2, "B. Second Debugger:FetchedVariables event received.");
          gDebugger.removeEventListener("Debugger:FetchedVariables", testB, false);
          Services.tm.currentThread.dispatch({ run: function() {

            var frames = gDebugger.DebuggerView.StackFrames._container._list,
                scopes = gDebugger.DebuggerView.Variables._list,
                innerScope = scopes.firstChild,
                innerNodes = innerScope.querySelector(".details").childNodes;

            is(gDebugger.DebuggerController.activeThread.state, "paused",
              "Should only be getting stack frames while paused.");

            is(frames.querySelectorAll(".dbg-stackframe").length, 1,
              "Should have one frame.");

            is(scopes.children.length, 3, "Should have 3 variable scopes.");

            is(innerNodes[0].querySelector(".name").getAttribute("value"), "<exception>",
              "Should have the right property name for the exception.");

            is(innerNodes[0].querySelector(".value").getAttribute("value"), "[object Error]",
              "Should have the right property value for the exception.");

            resumeAndFinish();
          }}, 0);
        }, false);
      });

      EventUtils.sendMouseEvent({ type: "mousedown" },
        gDebugger.document.getElementById("resume"),
        gDebugger);
    }, false);
  });

  EventUtils.sendMouseEvent({ type: "click" },
    content.document.querySelector("button"),
    content.window);
}

function resumeAndFinish() {
  gPane.contentWindow.gClient.addOneTimeListener("resumed", function() {
    Services.tm.currentThread.dispatch({ run: function() {

      closeDebuggerAndFinish(false);
    }}, 0);
  });

  
  gDebugger.DebuggerController.activeThread.resume();
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebugger = null;
});
