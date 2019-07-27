


"use strict";
















const CRASH_STATE = {windows: [{tabs: [{entries: [{url: "about:mozilla" }]}]}]};
const STATE = {entries: [createEntry(CRASH_STATE)]};
const STATE2 = {entries: [createEntry({windows: [{tabs: [STATE]}]})]};
const STATE3 = {entries: [createEntry(JSON.stringify(CRASH_STATE))]};

function createEntry(sessionData) {
  return {
    url: "about:sessionrestore",
    formdata: {id: {sessionData: sessionData}}
  };
}

add_task(function test_nested_about_sessionrestore() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  ss.setTabState(tab, JSON.stringify(STATE));
  yield promiseTabRestored(tab);
  checkState("test1", tab);

  
  ss.setTabState(tab, JSON.stringify(STATE2));
  yield promiseTabRestored(tab);
  checkState("test2", tab);

  
  ss.setTabState(tab, JSON.stringify(STATE3));
  yield promiseTabRestored(tab);
  checkState("test3", tab);

  
  gBrowser.removeTab(tab);
});

function checkState(prefix, tab) {
  
  TabState.flush(tab.linkedBrowser);
  let {formdata} = JSON.parse(ss.getTabState(tab));

  ok(formdata.id["sessionData"], prefix + ": we have form data for about:sessionrestore");

  let sessionData_raw = JSON.stringify(formdata.id["sessionData"]);
  ok(!/\\/.test(sessionData_raw), prefix + ": #sessionData contains no backslashes");
  info(sessionData_raw);

  let gotError = false;
  try {
    JSON.parse(formdata.id["sessionData"]);
  } catch (e) {
    info(prefix + ": got error: " + e);
    gotError = true;
  }
  ok(gotError, prefix + ": attempting to JSON.parse form data threw error");
}
