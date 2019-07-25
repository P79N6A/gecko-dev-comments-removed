


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    registerCleanupFunction(function () win.close());

    let cw = win.TabView.getContentWindow();
    let tab = win.gBrowser.tabs[0];
    let tabItem = tab._tabViewTabItem;
    let isIdle = false;

    
    
    let busyCount = 5;
    cw.UI.isIdle = function () {
      return isIdle = (0 > --busyCount);
    };

    cw.TabItems.pausePainting();

    tabItem.addSubscriber(tabItem, "updated", function () {
      tabItem.removeSubscriber(tabItem, "updated");
      ok(isIdle, "tabItem is updated only when UI is idle");
      finish();
    });

    cw.TabItems.update(tab);
    cw.TabItems.resumePainting();
  });
}
