






var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;
var gView = null;

function test() {
  debug_tab_pane(STACK_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.contentWindow;
    gView = gDebugger.DebuggerView;

    testPanesState();

    gView.togglePanes({ visible: true, animated: false });
    testPaneCollapse1();
    testPaneCollapse2();
    testPanesStartupPref(closeDebuggerAndFinish);
  });
}

function testPanesState() {
  let togglePanesButton =
    gDebugger.document.getElementById("toggle-panes");

  ok(togglePanesButton.getAttribute("panesHidden"),
    "The debugger view panes should initially be hidden.");
  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should not be checked.");
}

function testPaneCollapse1() {
  let stackframesAndBrekpoints =
    gDebugger.document.getElementById("stackframes+breakpoints");
  let togglePanesButton =
    gDebugger.document.getElementById("toggle-panes");

  let width = parseInt(stackframesAndBrekpoints.getAttribute("width"));
  is(width, gDebugger.Prefs.stackframesWidth,
    "The stackframes and breakpoints pane has an incorrect width.");
  is(stackframesAndBrekpoints.style.marginLeft, "0px",
    "The stackframes and breakpoints pane has an incorrect left margin.");
  ok(!stackframesAndBrekpoints.hasAttribute("animated"),
    "The stackframes and breakpoints pane has an incorrect animated attribute.");
  ok(!togglePanesButton.getAttribute("panesHidden"),
    "The stackframes and breakpoints pane should at this point be visible.");

  gView.togglePanes({ visible: false, animated: true });

  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should still initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should still not be checked.");

  let margin = -(width + 1) + "px";
  is(width, gDebugger.Prefs.stackframesWidth,
    "The stackframes and breakpoints pane has an incorrect width after collapsing.");
  is(stackframesAndBrekpoints.style.marginLeft, margin,
    "The stackframes and breakpoints pane has an incorrect left margin after collapsing.");
  ok(stackframesAndBrekpoints.hasAttribute("animated"),
    "The stackframes and breakpoints pane has an incorrect attribute after an animated collapsing.");
  ok(togglePanesButton.hasAttribute("panesHidden"),
    "The stackframes and breakpoints pane should not be visible after collapsing.");

  gView.togglePanes({ visible: true, animated: false });

  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should still initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should still not be checked.");

  is(width, gDebugger.Prefs.stackframesWidth,
    "The stackframes and breakpoints pane has an incorrect width after uncollapsing.");
  is(stackframesAndBrekpoints.style.marginLeft, "0px",
    "The stackframes and breakpoints pane has an incorrect left margin after uncollapsing.");
  ok(!stackframesAndBrekpoints.hasAttribute("animated"),
    "The stackframes and breakpoints pane has an incorrect attribute after an unanimated uncollapsing.");
  ok(!togglePanesButton.getAttribute("panesHidden"),
    "The stackframes and breakpoints pane should be visible again after uncollapsing.");
}

function testPaneCollapse2() {
  let variables =
    gDebugger.document.getElementById("variables");
  let togglePanesButton =
    gDebugger.document.getElementById("toggle-panes");

  let width = parseInt(variables.getAttribute("width"));
  is(width, gDebugger.Prefs.variablesWidth,
    "The variables pane has an incorrect width.");
  is(variables.style.marginRight, "0px",
    "The variables pane has an incorrect right margin.");
  ok(!variables.hasAttribute("animated"),
    "The variables pane has an incorrect animated attribute.");
  ok(!togglePanesButton.getAttribute("panesHidden"),
    "The variables pane should at this point be visible.");

  gView.togglePanes({ visible: false, animated: true });

  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should still initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should still not be checked.");

  let margin = -(width + 1) + "px";
  is(width, gDebugger.Prefs.variablesWidth,
    "The variables pane has an incorrect width after collapsing.");
  is(variables.style.marginRight, margin,
    "The variables pane has an incorrect right margin after collapsing.");
  ok(variables.hasAttribute("animated"),
    "The variables pane has an incorrect attribute after an animated collapsing.");
  ok(togglePanesButton.hasAttribute("panesHidden"),
    "The variables pane should not be visible after collapsing.");

  gView.togglePanes({ visible: true, animated: false });

  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should still initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should still not be checked.");

  is(width, gDebugger.Prefs.variablesWidth,
    "The variables pane has an incorrect width after uncollapsing.");
  is(variables.style.marginRight, "0px",
    "The variables pane has an incorrect right margin after uncollapsing.");
  ok(!variables.hasAttribute("animated"),
    "The variables pane has an incorrect attribute after an unanimated uncollapsing.");
  ok(!togglePanesButton.getAttribute("panesHidden"),
    "The variables pane should be visible again after uncollapsing.");
}

function testPanesStartupPref(aCallback) {
  let stackframesAndBrekpoints =
    gDebugger.document.getElementById("stackframes+breakpoints");
  let variables =
    gDebugger.document.getElementById("variables");
  let togglePanesButton =
    gDebugger.document.getElementById("toggle-panes");

  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should still initially be preffed as hidden.");

  ok(!togglePanesButton.getAttribute("panesHidden"),
    "The debugger panes should at this point be visible.");
  is(gDebugger.Prefs.panesVisibleOnStartup, false,
    "The debugger view panes should initially be preffed as hidden.");
  isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
    "The options menu item should still not be checked.");

  gDebugger.DebuggerView.Options._showPanesOnStartupItem.setAttribute("checked", "true");
  gDebugger.DebuggerView.Options._toggleShowPanesOnStartup();

  executeSoon(function() {
    ok(!togglePanesButton.getAttribute("panesHidden"),
      "The debugger panes should at this point be visible.");
    is(gDebugger.Prefs.panesVisibleOnStartup, true,
      "The debugger view panes should now be preffed as visible.");
    is(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
      "The options menu item should now be checked.");

    gDebugger.DebuggerView.Options._showPanesOnStartupItem.setAttribute("checked", "false");
    gDebugger.DebuggerView.Options._toggleShowPanesOnStartup();

    executeSoon(function() {
      ok(!togglePanesButton.getAttribute("panesHidden"),
        "The debugger panes should at this point be visible.");
      is(gDebugger.Prefs.panesVisibleOnStartup, false,
        "The debugger view panes should now be preffed as hidden.");
      isnot(gDebugger.DebuggerView.Options._showPanesOnStartupItem.getAttribute("checked"), "true",
        "The options menu item should now be unchecked.");

      executeSoon(function() {
        aCallback();
      });
    });
  });
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebuggee = null;
  gDebugger = null;
  gView = null;
});
