




































function test() {
  waitForExplicitFinish();

  newWindowWithTabView(onTabViewShown);
}

function onTabViewShown(win) {
  let contentWindow = win.TabView.getContentWindow();

  let getTransformValue = function (childIndex) {
    let tabItem = groupItem.getChildren()[childIndex];
    let style = contentWindow.getComputedStyle(tabItem.container);
    return style.getPropertyValue('-moz-transform');
  }

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

  contentWindow.GroupItems.setActiveGroupItem(groupItem);

  
  for (var i=0; i<7; i++)
    win.gBrowser.loadOneTab('about:blank', {inBackground: true});

  
  contentWindow.GroupItems.resumeArrange();

  ok(getTransformValue(5) != getTransformValue(4), 'the 6th child must peek out of its position under the 5th');
  is(getTransformValue(6), getTransformValue(5), 'the 7th child must not peek out of its position under the 6th');

  finishTest();
}
