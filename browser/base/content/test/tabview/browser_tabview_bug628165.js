




































function test() {
  let cw;

  let sendWindowsKey = function () {
    let utils = cw.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);
    utils.sendKeyEvent('keydown', 0, 0, 0);
  }

  let testEnableSearchWithWindowsKey = function () {
    sendWindowsKey();
    ok(!cw.isSearchEnabled(), 'search is not enabled');
    hideTabView(finish);
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    testEnableSearchWithWindowsKey();
  });
}
