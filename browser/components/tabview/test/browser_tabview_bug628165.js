


function test() {
  let cw;

  let testEnableSearchWithWindowsKey = function () {
    let utils = cw.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                  .getInterface(Components.interfaces.nsIDOMWindowUtils);

    
    let keyCodes = [0, 91, 92];
    keyCodes.forEach(function(keyCode) {
      utils.sendKeyEvent("keydown", keyCode, 0, 0);
      ok(!cw.Search.isEnabled(), "search is not enabled with keyCode: " + keyCode);
    });
    
    hideTabView(finish);
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    testEnableSearchWithWindowsKey();
  });
}
