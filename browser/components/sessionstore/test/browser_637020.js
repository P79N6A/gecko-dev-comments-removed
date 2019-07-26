


const TEST_URL = "http://mochi.test:8888/browser/browser/components/" +
                 "sessionstore/test/browser_637020_slow.sjs";

const TEST_STATE = {
  windows: [{
    tabs: [
      { entries: [{ url: "about:mozilla" }] },
      { entries: [{ url: "about:robots" }] }
    ]
  }, {
    tabs: [
      { entries: [{ url: TEST_URL }] },
      { entries: [{ url: TEST_URL }] }
    ]
  }]
};

function test() {
  TestRunner.run();
}











function runTests() {
  let win;

  
  Services.obs.addObserver(function onOpened(subject) {
    Services.obs.removeObserver(onOpened, "domwindowopened");
    win = subject;
    executeSoon(next);
  }, "domwindowopened", false);

  
  
  yield SessionStore.setBrowserState(JSON.stringify(TEST_STATE));

  
  
  info("the window has been opened");
  checkWindows();

  
  
  yield whenDelayedStartupFinished(win, next);
  info("the delayed startup has finished");
  checkWindows();
}

function checkWindows() {
  let state = JSON.parse(SessionStore.getBrowserState());
  is(state.windows[0].tabs.length, 2, "first window has two tabs");
  is(state.windows[1].tabs.length, 2, "second window has two tabs");
}
