






function test() {
  let monitor, reqMenu;
  initNetMonitor(SINGLE_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    monitor = aMonitor;
    let { document, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;
    reqMenu = RequestsMenu;

    is(reqMenu.itemCount, 0,
      "The request menu should empty before reloading");

    let button = document.querySelector("#requests-menu-reload-notice-button");
    button.click();
  })
  .then(() => {
    return waitForNetworkEvents(monitor, 2);
  })
  .then(() => {
    is(reqMenu.itemCount, 2,
      "The request menu should have two items after reloading");
  })
  .then(() => {
    return teardown(monitor).then(finish);
  });
}
