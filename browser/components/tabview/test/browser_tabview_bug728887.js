


function test() {
  waitForExplicitFinish();

  showTabView(function () {
    let cw = TabView.getContentWindow();
    let target = cw.GroupItems.groupItems[0].container;
    EventUtils.sendMouseEvent({type: "dblclick", button: 0}, target, cw);
    is(cw.GroupItems.groupItems.length, 1, "one groupItem after double clicking");

    hideTabView(finish);
  });
}
