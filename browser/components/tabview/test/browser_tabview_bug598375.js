


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(function (win) {
    registerCleanupFunction(function () win.close());

    let cw = win.TabView.getContentWindow();
    let groupItem = cw.GroupItems.groupItems[0];
    groupItem.setBounds(new cw.Rect(cw.innerWidth - 200, 0, 200, 200));

    whenTabViewIsHidden(function () waitForFocus(finish), win);

    waitForFocus(function () {
      let button = cw.document.getElementById("exit-button");
      EventUtils.synthesizeMouseAtCenter(button, {}, cw);
    }, cw);
  });

  
  
  ok(true, "Each test requires at least one pass, fail or todo so here is a pass.");
}
