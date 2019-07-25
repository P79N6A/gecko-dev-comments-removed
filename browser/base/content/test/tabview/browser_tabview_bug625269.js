


function test() {
  waitForExplicitFinish();
  newWindowWithTabView(onTabViewShown);
}

function onTabViewShown(win) {
  registerCleanupFunction(function () win.close());

  let contentWindow = win.TabView.getContentWindow();
  let currentGroup = contentWindow.GroupItems.getActiveGroupItem();

  function checkResized(diffX, diffY, shouldResize, text, callback) {
    let {width: origWidth, height: origHeight} = currentGroup.getBounds();

    resizeWindow(win, diffX, diffY, function () {
      let {width: newWidth, height: newHeight} = currentGroup.getBounds();
      let resized = (origWidth != newWidth || origHeight != newHeight);

      is(resized, shouldResize, text + ": The group should " +
         (shouldResize ? "" : "not ") + "have been resized");

      callback();
    });
  }

  function next() {
    let test = tests.shift();

    if (test)
      checkResized.apply(this, test.concat([next]));
    else
      finishTest();
  }

  function finishTest() {
    
    currentGroup.setSize(100, 100, true);
    currentGroup.setUserSize();
    checkResized(400, 400, false, "After clearing the cramp", finish);
  }

  let tests = [
    
    [ -50,  -50, false, "A little smaller"],
    [  50,   50, false, "A little bigger"],
    [-400, -400, true,  "Much smaller"],
    [ 400,  400, true,  "Bigger after much smaller"],
    [-400, -400, true,  "Much smaller"]
  ];

  
  currentGroup.setSize(600, 600, true);
  currentGroup.setUserSize();

  
  next();
}


function resizeWindow(win, diffX, diffY, callback) {
  let targetWidth = win.outerWidth + diffX;
  let targetHeight = win.outerHeight + diffY;

  win.addEventListener("resize", function onResize() {
    let {outerWidth: width, outerHeight: height} = win;
    if (width != targetWidth || height != targetHeight)
      return;

    win.removeEventListener("resize", onResize, false);
    executeSoon(callback);
  }, false);

  win.resizeBy(diffX, diffY);
}
