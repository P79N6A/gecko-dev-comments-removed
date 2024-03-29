



function test() {
  
  waitForExplicitFinish();

  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no");
  promiseWindowLoaded(newWin).then(() => {
    let newState = { windows: [{
      tabs: [{ entries: [] }],
      _closedTabs: [{
        state: { entries: [{ url: "about:" }]},
        title: "About:"
      }],
      sizemode: "maximized"
    }] };

    let uniqueKey = "bug 477657";
    let uniqueValue = "unik" + Date.now();

    ss.setWindowValue(newWin, uniqueKey, uniqueValue);
    is(ss.getWindowValue(newWin, uniqueKey), uniqueValue,
       "window value was set before the window was overwritten");
    ss.setWindowState(newWin, JSON.stringify(newState), true);

    
    newWin.setTimeout(function() {
      is(ss.getWindowValue(newWin, uniqueKey), "",
         "window value was implicitly cleared");

      is(newWin.windowState, newWin.STATE_MAXIMIZED,
         "the window was maximized");

      is(JSON.parse(ss.getClosedTabData(newWin)).length, 1,
         "the closed tab was added before the window was overwritten");
      delete newState.windows[0]._closedTabs;
      delete newState.windows[0].sizemode;
      ss.setWindowState(newWin, JSON.stringify(newState), true);

      newWin.setTimeout(function() {
        is(JSON.parse(ss.getClosedTabData(newWin)).length, 0,
           "closed tabs were implicitly cleared");

        is(newWin.windowState, newWin.STATE_MAXIMIZED,
           "the window remains maximized");
        newState.windows[0].sizemode = "normal";
        ss.setWindowState(newWin, JSON.stringify(newState), true);

        newWin.setTimeout(function() {
          isnot(newWin.windowState, newWin.STATE_MAXIMIZED,
                "the window was explicitly unmaximized");

          newWin.close();
          finish();
        }, 0);
      }, 0);
    }, 0);
  });
}
