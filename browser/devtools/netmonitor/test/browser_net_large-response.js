






function test() {
  initNetMonitor(CUSTOM_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    
    
    requestLongerTimeout(2);

    let { document, Editor, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    waitForNetworkEvents(aMonitor, 1).then(() => {
      verifyRequestItemTarget(RequestsMenu.getItemAtIndex(0),
        "GET", CONTENT_TYPE_SJS + "?fmt=html-long", {
          status: 200,
          statusText: "OK"
        });

      aMonitor.panelWin.once(aMonitor.panelWin.EVENTS.RESPONSE_BODY_DISPLAYED, () => {
        NetMonitorView.editor("#response-content-textarea").then((aEditor) => {
          ok(aEditor.getText().match(/^<p>/),
            "The text shown in the source editor is incorrect.");
          is(aEditor.getMode(), Editor.modes.text,
            "The mode active in the source editor is incorrect.");

          teardown(aMonitor).then(finish);
        });
      });

      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.getElementById("details-pane-toggle"));
      EventUtils.sendMouseEvent({ type: "mousedown" },
        document.querySelectorAll("#details-pane tab")[3]);
    });

    aDebuggee.performRequests(1, CONTENT_TYPE_SJS + "?fmt=html-long");
  });

  
  info("Forcing GC after netmonitor test.");
  Cu.forceGC();
}
