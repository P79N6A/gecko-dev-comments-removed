




































function test() {
  let pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);

  
  function countClosedTabsByTitle(aClosedTabList, aTitle)
    aClosedTabList.filter(function (aData) aData.title == aTitle).length;

  function countOpenTabsByTitle(aOpenTabList, aTitle)
    aOpenTabList.filter(function (aData) aData.entries.some(function (aEntry) aEntry.title == aTitle)).length

  
  let oldState = ss.getBrowserState();
  let oldState_wins = JSON.parse(oldState).windows.length;
  if (oldState_wins != 1)
    ok(false, "oldState in test_purge has " + oldState_wins + " windows instead of 1");

  
  const REMEMBER = Date.now(), FORGET = Math.random();
  let testState = {
    windows: [ { tabs: [{ entries: [{ url: "http://example.com/" }] }], selected: 1 } ],
    _closedWindows : [
      
      {
        tabs: [
          { entries: [{ url: "http://example.com/", title: REMEMBER }] },
          { entries: [{ url: "http://mozilla.org/", title: FORGET }] }
        ],
        selected: 2,
        title: "mozilla.org",
        _closedTabs: []
      },
      
      {
        tabs: [
         { entries: [{ url: "http://mozilla.org/", title: FORGET }] },
         { entries: [{ url: "http://example.com/", title: REMEMBER }] },
         { entries: [{ url: "http://example.com/", title: REMEMBER }] },
         { entries: [{ url: "http://mozilla.org/", title: FORGET }] },
         { entries: [{ url: "http://example.com/", title: REMEMBER }] }
        ],
        selected: 5,
        _closedTabs: []
      },
      
      {
        tabs: [
          { entries: [{ url: "http://example.com/", title: REMEMBER }] }
        ],
        selected: 1,
        _closedTabs: [
          {
            state: {
              entries: [
                { url: "http://mozilla.org/", title: FORGET },
                { url: "http://mozilla.org/again", title: "doesn't matter" }
              ]
            },
            pos: 1,
            title: FORGET
          },
          {
            state: {
              entries: [
                { url: "http://example.com", title: REMEMBER }
              ]
            },
            title: REMEMBER
          }
        ]
      }
    ]
  };
  
  
  ss.setBrowserState(JSON.stringify(testState));

  
  pb.removeDataFromDomain("mozilla.org");

  let closedWindowData = JSON.parse(ss.getClosedWindowData());

  
  let win = closedWindowData[0];
  is(win.tabs.length, 1, "1 tab was removed");
  is(countOpenTabsByTitle(win.tabs, FORGET), 0,
     "The correct tab was removed");
  is(countOpenTabsByTitle(win.tabs, REMEMBER), 1,
     "The correct tab was remembered");
  is(win.selected, 1, "Selected tab has changed");
  is(win.title, REMEMBER, "The window title was correctly updated");

  
  win = closedWindowData[1];
  is(win.tabs.length, 3, "2 tabs were removed");
  is(countOpenTabsByTitle(win.tabs, FORGET), 0,
     "The correct tabs were removed");
  is(countOpenTabsByTitle(win.tabs, REMEMBER), 3,
     "The correct tabs were remembered");
  is(win.selected, 3, "Selected tab has changed");
  is(win.title, REMEMBER, "The window title was correctly updated");

  
  win = closedWindowData[2];
  is(countClosedTabsByTitle(win._closedTabs, REMEMBER), 1,
     "The correct number of tabs were removed, and the correct ones");
  is(countClosedTabsByTitle(win._closedTabs, FORGET), 0,
     "All tabs to be forgotten were indeed removed");

  
  ss.setBrowserState(oldState);
}
