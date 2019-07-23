




































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();
  
  const REMEMBER = Date.now(), FORGET = Math.random();
  let test_state = {
    windows: [ { tabs: [{ entries: [{ url: "http://example.com/" }] }], selected: 1 } ],
    _closedWindows : [
      
      {
        tabs: [
          { entries: [{ url: "http://example.com/", title: "title" }] },
          { entries: [{ url: "http://mozilla.org/", title: "title" }] }
        ],
        selected: 2,
        title: FORGET,
        _closedTabs: []
      },
      
      {
        tabs: [
         { entries: [{ url: "http://mozilla.org/", title: "title" }] },
         { entries: [{ url: "http://example.com/", title: "title" }] },
         { entries: [{ url: "http://mozilla.org/", title: "title" }] },
        ],
        selected: 3,
        title: REMEMBER,
        _closedTabs: []
      },
      
      {
        tabs: [
          { entries: [{ url: "http://example.com/", title: "title" }] }
        ],
        selected: 1,
        title: FORGET,
        _closedTabs: [
          {
            state: {
              entries: [
                { url: "http://mozilla.org/", title: "title" },
                { url: "http://mozilla.org/again", title: "title" }
              ]
            },
            pos: 1,
            title: "title"
          },
          {
            state: {
              entries: [
                { url: "http://example.com", title: "title" }
              ]
            },
            title: "title"
          }
        ]
      }
    ]
  };
  let remember_count = 1;
  
  function countByTitle(aClosedWindowList, aTitle)
    aClosedWindowList.filter(function(aData) aData.title == aTitle).length;
  
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
    gPrefService.setIntPref("browser.sessionstore.max_windows_undo",
                            test_state._closedWindows.length);
    ss.setWindowState(newWin, JSON.stringify(test_state), true);
    
    let closedWindows = JSON.parse(ss.getClosedWindowData());
    is(closedWindows.length, test_state._closedWindows.length,
       "Closed window list has the expected length");
    is(countByTitle(closedWindows, FORGET),
       test_state._closedWindows.length - remember_count,
       "The correct amount of windows are to be forgotten");
    is(countByTitle(closedWindows, REMEMBER), remember_count,
       "Everything is set up.");
    
    
    ok(testForError(function() ss.forgetClosedWindow(-1)),
       "Invalid window for forgetClosedWindow throws");
    ok(testForError(function() ss.forgetClosedWindow(test_state._closedWindows.length + 1)),
       "Invalid window for forgetClosedWindow throws");
	   
    
    ss.forgetClosedWindow(2);
    ss.forgetClosedWindow(null);
    
    closedWindows = JSON.parse(ss.getClosedWindowData());
    is(closedWindows.length, remember_count,
       "The correct amount of windows were removed");
    is(countByTitle(closedWindows, FORGET), 0,
       "All windows specifically forgotten were indeed removed");
    is(countByTitle(closedWindows, REMEMBER), remember_count,
       "... and windows not specifically forgetten weren't.");

    
    newWin.close();
    if (gPrefService.prefHasUserValue("browser.sessionstore.max_windows_undo"))
      gPrefService.clearUserPref("browser.sessionstore.max_windows_undo");
    finish();
  }, false);
}
