


"use strict";
















const CRASH_STATE = {windows: [{tabs: [{entries: [{url: "about:mozilla" }]}]}]};
const STATE = createEntries(CRASH_STATE);
const STATE2 = createEntries({windows: [{tabs: [STATE]}]});
const STATE3 = createEntries(JSON.stringify(CRASH_STATE));

function createEntries(sessionData) {
  return {
    entries: [{url: "about:sessionrestore"}],
    formdata: {id: {sessionData: sessionData}, url: "about:sessionrestore"}
  };
}

add_task(function test_nested_about_sessionrestore() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield promiseTabState(tab, STATE);
  yield checkState("test1", tab);

  
  yield promiseTabState(tab, STATE2);
  yield checkState("test2", tab);

  
  yield promiseTabState(tab, STATE3);
  yield checkState("test3", tab);

  
  gBrowser.removeTab(tab);
});

function* checkState(prefix, tab) {
  
  yield TabStateFlusher.flush(tab.linkedBrowser);
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
