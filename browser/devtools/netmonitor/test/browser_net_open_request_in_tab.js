






function test() {
  waitForExplicitFinish();

  initNetMonitor(CUSTOM_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test...");

    let { NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;

    RequestsMenu.lazyUpdate = false;

    waitForNetworkEvents(aMonitor, 1).then(() => {
      let requestItem = RequestsMenu.getItemAtIndex(0);
      RequestsMenu.selectedItem = requestItem;

      gBrowser.tabContainer.addEventListener("TabOpen",function onOpen(event){
        ok(true, "A new tab has been opened ");
        gBrowser.tabContainer.removeEventListener("TabOpen", onOpen, false);
        cleanUp();
      }, false);

      RequestsMenu.openRequestInTab();
    });

    aDebuggee.performRequests(1);
    function cleanUp(){
      teardown(aMonitor).then(() => {
        gBrowser.removeCurrentTab();
        finish();
      });
    }
  });
}
