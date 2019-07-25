



let stateBackup = ss.getBrowserState();

function test() {
  
  waitForExplicitFinish();
  testBug600545();
}

function testBug600545() {
  
  Services.prefs.setBoolPref("browser.sessionstore.resume_from_crash", false);
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("browser.sessionstore.resume_from_crash");
    Services.prefs.clearUserPref("browser.sessionstore.interval");
  });

  
  
  
  
  
  let state = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#0" }], pinned:true },
        { entries: [{ url: "http://example.com#1" }] },
        { entries: [{ url: "http://example.com#2" }] },
      ],
      selected: 2
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#3" }] },
        { entries: [{ url: "http://example.com#4" }] },
        { entries: [{ url: "http://example.com#5" }] },
        { entries: [{ url: "http://example.com#6" }] }
      ],
      selected: 3
    }
  ] };

  waitForBrowserState(state, function() {
    
    
    waitForSaveState(function () {
      let expectedNumberOfTabs = getStateTabCount(state);
      let retrievedState = JSON.parse(ss.getBrowserState());
      let actualNumberOfTabs = getStateTabCount(retrievedState);

      is(actualNumberOfTabs, expectedNumberOfTabs,
        "Number of tabs in retreived session data, matches number of tabs set.");

      done();
    });
  });
}

function done() {
  
  
  let windowsEnum = Services.wm.getEnumerator("navigator:browser");
  while (windowsEnum.hasMoreElements()) {
    let currentWindow = windowsEnum.getNext();
    if (currentWindow != window)
      currentWindow.close();
  }

  ss.setBrowserState(stateBackup);
  executeSoon(finish);
}


function getStateTabCount(aState) {
  let tabCount = 0;
  for (let i in aState.windows)
    tabCount += aState.windows[i].tabs.length;
  return tabCount;
}
