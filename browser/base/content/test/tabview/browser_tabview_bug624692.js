


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    registerCleanupFunction(function () win.close());

    let cw = win.TabView.getContentWindow();
    let groupItem = cw.GroupItems.groupItems[0];
    let tabItem = groupItem.getChild(0);

    hideGroupItem(groupItem, function () {
      unhideGroupItem(groupItem, function () {
        let bounds = tabItem.getBounds();
        groupItem.arrange({immediately: true});
        ok(bounds.equals(tabItem.getBounds()),
           "tabItem bounds were correct after unhiding the groupItem");

        finish();
      });
    });
  });
}
