




































function provideWindow(aCallback, aURL, aFeatures) {
  function callback() {
    executeSoon(function () {
      aCallback(win);
    });
  }

  let win = openDialog(getBrowserURL(), "", aFeatures || "chrome,all,dialog=no", aURL);

  whenWindowLoaded(win, function () {
    if (!aURL) {
      callback();
      return;
    }
    win.gBrowser.selectedBrowser.addEventListener("load", function() {
      win.gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
      callback();
    }, true);
  });
}

function whenWindowLoaded(aWin, aCallback) {
  aWin.addEventListener("load", function () {
    aWin.removeEventListener("load", arguments.callee, false);
    executeSoon(function () {
      aCallback(aWin);
    });
  }, false);
}

function test() {
  
  
  
  requestLongerTimeout(2);

  waitForExplicitFinish();  

  
  function openWindowRec(windowsToOpen, expectedResults, recCallback) {
    
    if (!windowsToOpen.length) {
      let closedWindowData = JSON.parse(ss.getClosedWindowData());
      let numPopups = closedWindowData.filter(function(el, i, arr) {
        return el.isPopup;
      }).length;
      let numNormal = ss.getClosedWindowCount() - numPopups;
      
      let oResults = navigator.platform.match(/Mac/) ? expectedResults.mac
                                                     : expectedResults.other;
      is(numPopups, oResults.popup,
         "There were " + oResults.popup + " popup windows to repoen");
      is(numNormal, oResults.normal,
         "There were " + oResults.normal + " normal windows to repoen");

      
      executeSoon(recCallback);
      return;
    }
    
    let winData = windowsToOpen.shift();
    let settings = "chrome,dialog=no," +
                   (winData.isPopup ? "all=no" : "all");
    let url = "http://example.com/?window=" + windowsToOpen.length;

    provideWindow(function (win) {
      win.close();
      openWindowRec(windowsToOpen, expectedResults, recCallback);
    }, url, settings);
  }

  let windowsToOpen = [{isPopup: false},
                       {isPopup: false},
                       {isPopup: true},
                       {isPopup: true},
                       {isPopup: true}];
  let expectedResults = {mac: {popup: 3, normal: 0},
                         other: {popup: 3, normal: 1}};
  let windowsToOpen2 = [{isPopup: false},
                        {isPopup: false},
                        {isPopup: false},
                        {isPopup: false},
                        {isPopup: false}];
  let expectedResults2 = {mac: {popup: 0, normal: 3},
                          other: {popup: 0, normal: 3}};
  openWindowRec(windowsToOpen, expectedResults, function() {
    openWindowRec(windowsToOpen2, expectedResults2, finish);
  });
}
