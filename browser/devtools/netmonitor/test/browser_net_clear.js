






function test() {
  initNetMonitor(SIMPLE_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    let { document, $, NetMonitorView } = aMonitor.panelWin;
    let { RequestsMenu } = NetMonitorView;
    let detailsPane = $("#details-pane");
    let detailsPaneToggleButton = $('#details-pane-toggle');
    let clearButton = $('#requests-menu-clear-button');

    RequestsMenu.lazyUpdate = false;

    
    assertNoRequestState(RequestsMenu, detailsPaneToggleButton);

    
    aMonitor.panelWin.once(aMonitor.panelWin.EVENTS.NETWORK_EVENT, () => {
      assertSingleRequestState(RequestsMenu, detailsPaneToggleButton);

      
      EventUtils.sendMouseEvent({ type: "click" }, clearButton);
      assertNoRequestState(RequestsMenu, detailsPaneToggleButton);

      
      aMonitor.panelWin.once(aMonitor.panelWin.EVENTS.NETWORK_EVENT, () => {
        assertSingleRequestState(RequestsMenu, detailsPaneToggleButton);

        
        NetMonitorView.toggleDetailsPane({ visible: true, animated: false });
        ok(!detailsPane.hasAttribute("pane-collapsed") &&
          !detailsPaneToggleButton.hasAttribute("pane-collapsed"),
          "The details pane should be visible after clicking the toggle button.");

        
        EventUtils.sendMouseEvent({ type: "click" }, clearButton);
        assertNoRequestState(RequestsMenu, detailsPaneToggleButton);
        ok(detailsPane.hasAttribute("pane-collapsed") &&
          detailsPaneToggleButton.hasAttribute("pane-collapsed"),
          "The details pane should not be visible clicking 'clear'.");

        teardown(aMonitor).then(finish);
      });

      aDebuggee.location.reload();
    });

    aDebuggee.location.reload();
  });

  


  function assertSingleRequestState(RequestsMenu, detailsPaneToggleButton) {
    is(RequestsMenu.itemCount, 1,
      "The request menu should have one item at this point.");
    is(detailsPaneToggleButton.hasAttribute("disabled"), false,
      "The pane toggle button should be enabled after a request is made.");
  }

  


  function assertNoRequestState(RequestsMenu, detailsPaneToggleButton) {
    is(RequestsMenu.itemCount, 0,
      "The request menu should be empty at this point.");
    is(detailsPaneToggleButton.hasAttribute("disabled"), true,
      "The pane toggle button should be disabled when the request menu is cleared.");
  }
}
