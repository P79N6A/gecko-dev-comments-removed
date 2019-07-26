







function test() {
  let monitor, reqMenu;
  initNetMonitor(SINGLE_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    monitor = aMonitor;
    let { document, NetMonitorView, NetMonitorController } = aMonitor.panelWin;
    let { RequestsMenu, NetworkDetails } = NetMonitorView;
    reqMenu = RequestsMenu;

    Services.prefs.setBoolPref("devtools.webconsole.persistlog", false);
    content.location.reload(true);
  })
  .then(() => {
    return waitForNetworkEvents(monitor, 2);
  })
  .then(() => {
    is(reqMenu.itemCount, 2,
      "The request menu should have two items at this point.");
  })
  .then(() => {
     content.location.reload(true);
     return waitForNetworkEvents(monitor, 2);
   })
  .then(() => {
    
    is(reqMenu.itemCount, 2,
      "The request menu should still have two items at this point.");
  })
  .then(() => {
    
     Services.prefs.setBoolPref("devtools.webconsole.persistlog", true);
     content.location.reload(true);
     return waitForNetworkEvents(monitor, 2);
   })
  .then(() => {
    
    is(reqMenu.itemCount, 4,
      "The request menu should now have four items at this point.");
  })
  .then(() => {
    Services.prefs.setBoolPref("devtools.webconsole.persistlog", false);
    return teardown(monitor).then(finish);
  });
}
