







function test() {
  initNetMonitor(CYRILLIC_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    let { document, Editor, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    waitForNetworkEvents(aMonitor, 1).then(() => {
      verifyRequestItemTarget(RequestsMenu.getItemAtIndex(0),
        "GET", CYRILLIC_URL, {
          status: 200,
          statusText: "OK"
        });

      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.getElementById("details-pane-toggle"));
      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.querySelectorAll("#details-pane tab")[3]);

      NetMonitorView.editor("#response-content-textarea").then((aEditor) => {
        is(aEditor.getText().indexOf("\u044F"), 302, 
          "The text shown in the source editor is incorrect.");
        is(aEditor.getMode(), Editor.modes.html,
          "The mode active in the source editor is incorrect.");

        teardown(aMonitor).then(finish);
      });
    });

    aDebuggee.location.reload();
  });
}
