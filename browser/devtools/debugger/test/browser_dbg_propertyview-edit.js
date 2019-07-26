









const TAB_URL = EXAMPLE_URL + "browser_dbg_frame-parameters.html";

var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;

requestLongerTimeout(2);

function test() {
  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.contentWindow;

    gDebugger.DebuggerController.StackFrames.autoScopeExpand = true;
    gDebugger.DebuggerView.Variables.nonEnumVisible = false;
    testFrameEval();
  });
}

function testFrameEval() {
  gDebugger.addEventListener("Debugger:FetchedVariables", function test() {
    gDebugger.removeEventListener("Debugger:FetchedVariables", test, false);
    Services.tm.currentThread.dispatch({ run: function() {

      is(gDebugger.DebuggerController.activeThread.state, "paused",
        "Should only be getting stack frames while paused.");

      var localScope = gDebugger.DebuggerView.Variables._list.querySelector(".scope"),
          localNodes = localScope.querySelector(".details").childNodes,
          varA = localNodes[7];

      is(varA.querySelector(".name").getAttribute("value"), "a",
        "Should have the right name for 'a'.");

      is(varA.querySelector(".value").getAttribute("value"), 1,
        "Should have the right initial value for 'a'.");

      testModification(varA, function(aVar) {
        testModification(aVar, function(aVar) {
          testModification(aVar, function(aVar) {
            resumeAndFinish();
          }, "document.title", '"Debugger Function Call Parameter Test"');
        }, "b", "[object Object]");
      }, "{ a: 1 }", "[object Object]");
    }}, 0);
  }, false);

  EventUtils.sendMouseEvent({ type: "click" },
    content.document.querySelector("button"),
    content.window);
}

function testModification(aVar, aCallback, aNewValue, aNewResult) {
  function makeChangesAndExitInputMode() {
    EventUtils.sendString(aNewValue);
    EventUtils.sendKey("RETURN");
  }

  EventUtils.sendMouseEvent({ type: "click" },
    aVar.querySelector(".value"),
    gDebugger);

  executeSoon(function() {
    ok(aVar.querySelector(".element-value-input"),
      "There should be an input element created.");

    let count = 0;
    gDebugger.addEventListener("Debugger:FetchedVariables", function test() {
      
      
      if (++count < 2) {
        info("Number of received Debugger:FetchedVariables events: " + count);
        return;
      }
      gDebugger.removeEventListener("Debugger:FetchedVariables", test, false);
      
      
      var localScope = gDebugger.DebuggerView.Variables._list.querySelector(".scope"),
          localNodes = localScope.querySelector(".details").childNodes,
          varA = localNodes[7];

      is(varA.querySelector(".value").getAttribute("value"), aNewResult,
        "Should have the right value for 'a'.");

      executeSoon(function() {
        aCallback(varA);
      });
    }, false);

    makeChangesAndExitInputMode();
  });
}

function resumeAndFinish() {
  gDebugger.DebuggerController.activeThread.resume(function() {
    closeDebuggerAndFinish();
  });
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
});
