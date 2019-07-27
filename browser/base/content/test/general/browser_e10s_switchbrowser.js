const DUMMY_PATH = "browser/browser/base/content/test/general/dummy_page.html";

const gExpectedHistory = {
  index: -1,
  entries: []
};

function check_history() {
  let webNav = gBrowser.webNavigation;
  let sessionHistory = webNav.sessionHistory;

  let count = sessionHistory.count;
  is(count, gExpectedHistory.entries.length, "Should have the right number of history entries");
  is(sessionHistory.index, gExpectedHistory.index, "Should have the right history index");

  for (let i = 0; i < count; i++) {
    let entry = sessionHistory.getEntryAtIndex(i, false);
    is(entry.URI.spec, gExpectedHistory.entries[i].uri, "Should have the right URI");
    is(entry.title, gExpectedHistory.entries[i].title, "Should have the right title");
  }
}


let waitForLoad = Task.async(function*(uri) {
  info("Loading " + uri);
  gBrowser.loadURI(uri);

  yield waitForDocLoadComplete();
  gExpectedHistory.index++;
  gExpectedHistory.entries.push({
    uri: gBrowser.currentURI.spec,
    title: gBrowser.contentTitle
  });
});

let back = Task.async(function*() {
  info("Going back");
  gBrowser.goBack();
  yield waitForDocLoadComplete();
  gExpectedHistory.index--;
});

let forward = Task.async(function*() {
  info("Going forward");
  gBrowser.goForward();
  yield waitForDocLoadComplete();
  gExpectedHistory.index++;
});



add_task(function*() {
  SimpleTest.requestCompleteLog();

  let remoting = Services.prefs.getBoolPref("browser.tabs.remote.autostart");
  let expectedRemote = remoting ? "true" : "";

  info("1");
  
  gBrowser.selectedTab = gBrowser.addTab("about:blank", {skipAnimation: true});
  yield waitForLoad("http://example.org/" + DUMMY_PATH);
  is(gBrowser.selectedTab.getAttribute("remote"), expectedRemote, "Remote attribute should be correct");

  info("2");
  
  yield waitForLoad("http://example.com/" + DUMMY_PATH);
  is(gBrowser.selectedTab.getAttribute("remote"), expectedRemote, "Remote attribute should be correct");
  check_history();

  info("3");
  
  yield waitForLoad("about:robots");
  is(gBrowser.selectedTab.getAttribute("remote"), "", "Remote attribute should be correct");
  check_history();

  info("4");
  
  yield waitForLoad("http://example.org/" + DUMMY_PATH);
  is(gBrowser.selectedTab.getAttribute("remote"), expectedRemote, "Remote attribute should be correct");
  check_history();

  info("5");
  yield back();
  is(gBrowser.selectedTab.getAttribute("remote"), "", "Remote attribute should be correct");
  check_history();

  info("6");
  yield back();
  is(gBrowser.selectedTab.getAttribute("remote"), expectedRemote, "Remote attribute should be correct");
  check_history();

  info("7");
  yield forward();
  is(gBrowser.selectedTab.getAttribute("remote"), "", "Remote attribute should be correct");
  check_history();

  info("8");
  yield forward();
  is(gBrowser.selectedTab.getAttribute("remote"), expectedRemote, "Remote attribute should be correct");
  check_history();

  info("9");
  gBrowser.removeCurrentTab();
});
