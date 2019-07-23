



































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();
  
  let uniqueKey1 = "bug 465223.1";
  let uniqueKey2 = "bug 465223.2";
  let uniqueValue1 = "unik" + Date.now();
  let uniqueValue2 = "pi != " + Math.random();
  
  
  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no");
  newWin.addEventListener("load", function(aEvent) {
    ss.setWindowValue(newWin, uniqueKey1, uniqueValue1);
    
    let newState = { windows: [{ tabs:[{ entries: [] }], extData: {} }] };
    newState.windows[0].extData[uniqueKey2] = uniqueValue2;
    ss.setWindowState(newWin, JSON.stringify(newState), false);
    
    is(newWin.gBrowser.tabs.length, 2,
       "original tab wasn't overwritten");
    is(ss.getWindowValue(newWin, uniqueKey1), uniqueValue1,
       "window value wasn't overwritten when the tabs weren't");
    is(ss.getWindowValue(newWin, uniqueKey2), uniqueValue2,
       "new window value was correctly added");
    
    newState.windows[0].extData[uniqueKey2] = uniqueValue1;
    ss.setWindowState(newWin, JSON.stringify(newState), true);
    
    is(newWin.gBrowser.tabs.length, 1,
       "original tabs were overwritten");
    is(ss.getWindowValue(newWin, uniqueKey1), "",
       "window value was cleared");
    is(ss.getWindowValue(newWin, uniqueKey2), uniqueValue1,
       "window value was correctly overwritten");
    
    
    newWin.close();
    is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
    finish();
  }, false);
}
