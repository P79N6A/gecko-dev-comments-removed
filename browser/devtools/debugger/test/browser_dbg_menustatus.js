








function test() {
  var tab1 = addTab("about:blank", function() {
    var tab2 = addTab("about:blank", function() {
      gBrowser.selectedTab = tab2;

      let pane = DebuggerUI.toggleDebugger();
      ok(pane, "toggleDebugger() should return a pane.");
      let frame = pane._frame;

      frame.addEventListener("Debugger:Loaded", function dbgLoaded() {
        frame.removeEventListener("Debugger:Loaded", dbgLoaded, true);

        let cmd = document.getElementById("Tools:Debugger");
        is(cmd.getAttribute("checked"), "true", "<command Tools:Debugger> is checked.");

        gBrowser.selectedTab = tab1;

        is(cmd.getAttribute("checked"), "false", "<command Tools:Debugger> is unchecked after tab switch.");

        gBrowser.selectedTab = tab2;

        is(cmd.getAttribute("checked"), "true", "<command Tools:Debugger> is checked.");

        let pane = DebuggerUI.toggleDebugger();

        is(cmd.getAttribute("checked"), "false", "<command Tools:Debugger> is unchecked once closed.");
      }, true);

      frame.addEventListener("Debugger:Unloaded", function dbgUnloaded() {
        frame.removeEventListener("Debugger:Unloaded", dbgUnloaded, true);
          removeTab(tab1);
          removeTab(tab2);

          finish();
      }, true);
    });
  });
}

