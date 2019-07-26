






function test() {
  initNetMonitor(CYRILLIC_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    let { document, Editor, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    waitForNetworkEvents(aMonitor, 1).then(() => {
      verifyRequestItemTarget(RequestsMenu.getItemAtIndex(0),
        "GET", CONTENT_TYPE_SJS + "?fmt=txt", {
          status: 200,
          statusText: "DA DA DA"
        });

      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.getElementById("details-pane-toggle"));
      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.querySelectorAll("#details-pane tab")[3]);

      NetMonitorView.editor("#response-content-textarea").then((aEditor) => {
        is(aEditor.getText().indexOf("\u044F"), 26, 
          "The text shown in the source editor is incorrect.");
        is(aEditor.getMode(), Editor.modes.text,
          "The mode active in the source editor is incorrect.");

        teardown(aMonitor).then(finish);
      });
    });

    aDebuggee.performRequests();
  });
}
