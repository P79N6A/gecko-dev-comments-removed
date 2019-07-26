




var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;

function test() {
  debug_tab_pane(STACK_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.contentWindow;

    testNonEnumProperties();
  });
}

function testNonEnumProperties() {
  gDebugger.DebuggerController.activeThread.addOneTimeListener("framesadded", function() {
    Services.tm.currentThread.dispatch({ run: function() {
      let testScope = gDebugger.DebuggerView.Variables.addScope("test-scope");
      let testVar = testScope.addVar("foo");
      testVar.addProperties({
        foo: {
          value: "bar",
          enumerable: true
        },

        bar: {
          value: "foo",
          enumerable: false
        }
      });

      testScope.expand();
      testVar.expand();

      let details = testVar._enum;
      let nonenum = testVar._nonenum;

      is(details.childNodes.length, 1,
        "There should be just one property in the .details container.");

      ok(details.hasAttribute("open"),
        ".details container should be visible.");

      is(nonenum.childNodes.length, 1,
        "There should be just one property in the .nonenum container.");

      ok(nonenum.hasAttribute("open"),
        ".nonenum container should be visible.");

      
      gDebugger.DebuggerView.Options._showNonEnumItem.setAttribute("checked", "false");
      gDebugger.DebuggerView.Options._toggleShowNonEnum();

      ok(details.hasAttribute("open"),
        ".details container should stay visible.");

      ok(!nonenum.hasAttribute("open"),
        ".nonenum container should become hidden.");

      
      gDebugger.DebuggerView.Options._showNonEnumItem.setAttribute("checked", "true");
      gDebugger.DebuggerView.Options._toggleShowNonEnum();

      ok(details.hasAttribute("open"),
        ".details container should stay visible.");

      ok(nonenum.hasAttribute("open"),
        ".nonenum container should become visible.");

      testVar.collapse();

      ok(!details.hasAttribute("open"),
        ".details container should be hidden.");

      ok(!nonenum.hasAttribute("open"),
        ".nonenum container should be hidden.");

      
      gDebugger.DebuggerView.Options._showNonEnumItem.setAttribute("checked", "false");
      gDebugger.DebuggerView.Options._toggleShowNonEnum();

      ok(!details.hasAttribute("open"),
        ".details container should stay hidden.");

      ok(!nonenum.hasAttribute("open"),
        ".nonenum container should stay hidden.");

      
      gDebugger.DebuggerView.Options._showNonEnumItem.setAttribute("checked", "true");
      gDebugger.DebuggerView.Options._toggleShowNonEnum();

      gDebugger.DebuggerController.activeThread.resume(function() {
        closeDebuggerAndFinish();
      });
    }}, 0);
  });

  gDebuggee.simpleCall();
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
});
