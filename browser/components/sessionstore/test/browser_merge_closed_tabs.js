






add_task(function () {
  const initialState = {
    windows: [{
      tabs: [
        { entries: [{ url: "about:blank" }] }
      ],
      _closedTabs: [
        { state: { entries: [{ ID: 1000, url: "about:blank" }]} },
        { state: { entries: [{ ID: 1001, url: "about:blank" }]} }
      ]
    }]
  }

  const restoreState = {
    windows: [{
      tabs: [
        { entries: [{ url: "about:robots" }] }
      ],
      _closedTabs: [
        { state: { entries: [{ ID: 1002, url: "about:robots" }]} },
        { state: { entries: [{ ID: 1003, url: "about:robots" }]} },
        { state: { entries: [{ ID: 1004, url: "about:robots" }]} }
      ]
    }]
  }

  const maxTabsUndo = 4;
  gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", maxTabsUndo);

  
  let win = yield promiseNewWindowLoaded({private: false});
  SessionStore.setWindowState(win, JSON.stringify(initialState), true);
  is(SessionStore.getClosedTabCount(win), 2, "2 closed tabs after restoring initial state");

  
  
  SessionStore.setWindowState(win, JSON.stringify(restoreState), false);

  
  let iClosed = initialState.windows[0]._closedTabs;
  let rClosed = restoreState.windows[0]._closedTabs;
  let cData = JSON.parse(SessionStore.getClosedTabData(win));

  is(cData.length, Math.min(iClosed.length + rClosed.length, maxTabsUndo),
     "Number of closed tabs is correct");

  
  
  for (let i = 0; i < cData.length; i++) {
    if (i < rClosed.length) {
      is(cData[i].state.entries[0].ID, rClosed[i].state.entries[0].ID,
         "Closed tab entry matches");
    } else {
      is(cData[i].state.entries[0].ID, iClosed[i - rClosed.length].state.entries[0].ID,
         "Closed tab entry matches");
    }
  }

  
  gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");
  win.close();
});


