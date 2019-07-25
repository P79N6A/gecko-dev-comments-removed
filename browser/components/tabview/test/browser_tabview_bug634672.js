


function test() {
  let cw;
  let win;

  waitForExplicitFinish();

  newWindowWithTabView(function (tvwin) {
    win = tvwin;
    cw = win.TabView.getContentWindow();

    registerCleanupFunction(function () {
      if (win && !win.closed)
        win.close();
    });

    
    for (let i = 0; i < 5; i++)
      win.gBrowser.loadOneTab("about:blank");

    let groupItem = cw.GroupItems.groupItems[0];
    groupItem.setSize(400, 400, true);
    let range = new cw.Range(1, 400);

    
    while (range.extent > 1) {
      let pivot = Math.floor(range.extent / 2);
      groupItem.setSize(range.min + pivot, range.min + pivot, true);

      if (groupItem.isStacked())
        range.min += pivot;
      else
        range.max -= pivot;
    }

    
    groupItem.setSize(range.min, range.min, true);
    ok(groupItem.isStacked(), "groupItem is stacked");

    
    groupItem.setSize(range.max, range.max, true);
    ok(!groupItem.isStacked(), "groupItem is no longer stacked");

    
    let tabItem = groupItem.getChild(0);
    isnot(tabItem.$close.css("display"), "none", "close button is visible");

    finish();
  });
}
