




































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();
  
  const REMEMBER = Date.now(), FORGET = Math.random();
  let test_state = { windows: [{ "tabs": [{ "entries": [] }], _closedTabs: [
    { state: { entries: [{ url: "http://www.example.net/" }] }, title: FORGET },
    { state: { entries: [{ url: "http://www.example.net/" }] }, title: REMEMBER },
    { state: { entries: [{ url: "http://www.example.net/" }] }, title: FORGET },
    { state: { entries: [{ url: "http://www.example.net/" }] }, title: REMEMBER },
  ] }] };
  let remember_count = 2;
  
  function countByTitle(aClosedTabList, aTitle)
    aClosedTabList.filter(function(aData) aData.title == aTitle).length;
  
  function testForError(aFunction) {
    try {
      aFunction();
      return false;
    }
    catch (ex) {
      return ex.name == "NS_ERROR_ILLEGAL_VALUE";
    }
  }
  
  
  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no");
  newWin.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, false);
    gPrefService.setIntPref("browser.sessionstore.max_tabs_undo",
                            test_state.windows[0]._closedTabs.length);
    ss.setWindowState(newWin, JSON.stringify(test_state), true);
    
    let closedTabs = eval("(" + ss.getClosedTabData(newWin) + ")");
    is(closedTabs.length, test_state.windows[0]._closedTabs.length,
       "Closed tab list has the expected length");
    is(countByTitle(closedTabs, FORGET),
       test_state.windows[0]._closedTabs.length - remember_count,
       "The correct amout of tabs are to be forgotten");
    is(countByTitle(closedTabs, REMEMBER), remember_count,
       "Everything is set up.");
    
    
    ok(testForError(function() ss.forgetClosedTab({}, 0)),
       "Invalid window for forgetClosedTab throws");
    ok(testForError(function() ss.forgetClosedTab(newWin, -1)),
       "Invalid tab for forgetClosedTab throws");
    ok(testForError(function() ss.forgetClosedTab(newWin, test_state.windows[0]._closedTabs.length + 1)),
       "Invalid tab for forgetClosedTab throws");
	   
    
    ss.forgetClosedTab(newWin, 2);
    ss.forgetClosedTab(newWin, null);
    
    closedTabs = eval("(" + ss.getClosedTabData(newWin) + ")");
    is(closedTabs.length, remember_count,
       "The correct amout of tabs was removed");
    is(countByTitle(closedTabs, FORGET), 0,
       "All tabs specifically forgotten were indeed removed");
    is(countByTitle(closedTabs, REMEMBER), remember_count,
       "... and tabs not specifically forgetten weren't.");

    
    newWin.close();
    gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");
    finish();
  }, false);
}
