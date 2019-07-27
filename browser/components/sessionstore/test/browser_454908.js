


"use strict";

const URL = ROOT + "browser_454908_sample.html";
const PASS = "pwd-" + Math.random();




add_task(function* test_dont_save_passwords() {
  
  Services.prefs.clearUserPref("browser.sessionstore.privacy_level");

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let usernameValue = "User " + Math.random();
  yield setInputValue(browser, {id: "username", value: usernameValue});
  yield setInputValue(browser, {id: "passwd", value: PASS});

  
  yield promiseRemoveTab(tab);
  tab = ss.undoCloseTab(window, 0);
  browser = tab.linkedBrowser;
  yield promiseTabRestored(tab);

  
  let username = yield getInputValue(browser, {id: "username"});
  is(username, usernameValue, "username was saved/restored");
  let passwd = yield getInputValue(browser, {id: "passwd"});
  is(passwd, "", "password wasn't saved/restored");

  
  yield forceSaveState();
  yield promiseForEachSessionRestoreFile((state, key) =>
    
    ok(!state.includes(PASS), "password has not been written to file " + key)
  );

  
  gBrowser.removeTab(tab);
});
