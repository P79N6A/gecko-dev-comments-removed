






function test() {
  initNetMonitor(JSON_LONG_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    let { L10N, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    function verifyRequest(aOffset) {
      verifyRequestItemTarget(RequestsMenu.getItemAtIndex(aOffset),
        "GET", CONTENT_TYPE_SJS + "?fmt=json-long", {
          status: 200,
          statusText: "OK",
          type: "json",
          fullMimeType: "text/json; charset=utf-8",
          size: L10N.getFormatStr("networkMenu.sizeKB", L10N.numberWithDecimals(85975/1024, 2)),
          time: true
        });
    }

    waitForNetworkEvents(aMonitor, 1).then(() => {
      verifyRequest(0);

      aMonitor._toolbox.once("webconsole-selected", () => {
        aMonitor._toolbox.once("netmonitor-selected", () => {

          waitForNetworkEvents(aMonitor, 1).then(() => {
            waitForNetworkEvents(aMonitor, 1).then(() => {
              verifyRequest(1);
              teardown(aMonitor).then(finish);
            });

            
            aDebuggee.performRequests();
          });

          
          aDebuggee.location.reload();
        });

        
        aMonitor._toolbox.selectTool("netmonitor");
      });

      
      aMonitor._toolbox.selectTool("webconsole");
    });

    
    aDebuggee.performRequests();
  });
}
