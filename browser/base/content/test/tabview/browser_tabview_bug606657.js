




































function test() {
  waitForExplicitFinish();

  window.addEventListener('tabviewshown', onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener('tabviewshown', onTabViewWindowLoaded, false);

  let [tab] = gBrowser.tabs;
  let groupId = tab._tabViewTabItem.parent.id;

  let finishTest = function () {
    let onTabViewHidden = function () {
      window.removeEventListener('tabviewhidden', onTabViewHidden, false);
      finish();
    }

    window.addEventListener('tabviewhidden', onTabViewHidden, false);
    TabView.hide();
  }

  TabView.moveTabTo(tab, groupId);
  is(tab._tabViewTabItem.parent.id, groupId, 'tab did not change its group');

  finishTest();
}
