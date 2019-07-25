


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(onTabViewShown);
}

function onTabViewShown(win) {
  let contentWindow = win.TabView.getContentWindow();

  let finishTest = function () {
    win.addEventListener('tabviewhidden', function () {
      win.removeEventListener('tabviewhidden', arguments.callee, false);
      win.close();
      finish();
    }, false);
    win.TabView.hide();
  }

  
  contentWindow.GroupItems.pauseArrange();

  
  let groupItem = new contentWindow.GroupItem([], {
    immediately: true,
    bounds: {left: 20, top: 20, width: 100, height: 100}
  });

  contentWindow.UI.setActive(groupItem);

  
  for (var i=0; i<7; i++)
    win.gBrowser.loadOneTab('about:blank', {inBackground: true});

  
  contentWindow.GroupItems.resumeArrange();

  let tabItem6 = groupItem.getChildren()[5];
  let tabItem7 = groupItem.getChildren()[6];  
  ok(!tabItem6.getHidden(), 'the 6th child must not be hidden');
  ok(tabItem7.getHidden(), 'the 7th child must be hidden');

  finishTest();
}
