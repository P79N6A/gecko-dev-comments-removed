





function test() {
  var tab1 = addTab(TAB1_URL, function() {
    gBrowser.selectedTab = tab1;

    ok(!DebuggerUI.getDebugger(gBrowser.selectedTab),
      "Shouldn't have a debugger pane for this tab yet.");

    let pane = DebuggerUI.startDebugger();
    let someHeight = parseInt(Math.random() * 200) + 200;

    ok(pane, "startDebugger() should return a pane.");

    is(DebuggerUI.getDebugger(gBrowser.selectedTab), pane,
      "getDebugger() should return the same pane as startDebugger().");

    ok(DebuggerUI.getPreferences().height,
      "The debugger preferences should have a saved height value.");

    is(DebuggerUI.getPreferences().height, pane.frame.height,
      "The debugger pane height should be the same as the preferred value.");

    pane.frame.height = someHeight;
    ok(DebuggerUI.getPreferences().height !== someHeight,
      "Height preferences shouldn't have been updated yet.");

    pane.onConnected = function() {
      removeTab(tab1);
      finish();

      is(DebuggerUI.getPreferences().height, someHeight,
        "Height preferences should have been updated by now.");
    };
  });
}
