


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    let cw = win.TabView.getContentWindow();
    let tabItem = win.gBrowser.tabs[0]._tabViewTabItem;

    tabItem.addSubscriber("savedCachedImageData", function onSaved() {
      tabItem.removeSubscriber("savedCachedImageData", onSaved);

      ok(cw.UI.isDOMWindowClosing, "dom window is closing");
      waitForFocus(finish);
    });

    win.close();
  });
}
