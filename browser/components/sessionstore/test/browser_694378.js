






function test() {
  waitForExplicitFinish();

  let brokenState = {
    windows: [
      { tabs: [{ entries: [{ url: "about:mozilla" }] }] }
      
    ],
    selectedWindow: 2
  };
  let brokenStateString = JSON.stringify(brokenState);

  let gotError = false;
  try {
    ss.setWindowState(window, brokenStateString, true);
  }
  catch (ex) {
    gotError = true;
    info(ex);
  }

  ok(!gotError, "ss.setWindowState did not throw an error");

  
  let blankState = { windows: [{ tabs: [{ entries: [{ url: "about:blank" }] }]}]};
  waitForBrowserState(blankState, finish);
}

