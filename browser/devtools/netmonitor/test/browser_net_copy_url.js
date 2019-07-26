






function test() {
  initNetMonitor(CUSTOM_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    let { NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    waitForNetworkEvents(aMonitor, 1).then(() => {
      let imageRequest = RequestsMenu.getItemAtIndex(0);
      RequestsMenu.selectedItem = imageRequest;

      waitForClipboard(RequestsMenu.selectedItem.attachment.url, function(){ RequestsMenu.copyUrl() } , cleanUp, cleanUp);
    });

    aDebuggee.performRequests(1);

    function cleanUp(){
      teardown(aMonitor);
      finish();
    }
  });
}

