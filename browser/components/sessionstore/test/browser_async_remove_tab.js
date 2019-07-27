"use strict";

function* createTabWithRandomValue(url) {
  let tab = gBrowser.addTab(url);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  let r = `rand-${Math.random()}`;
  ss.setTabValue(tab, "foobar", r);

  
  yield TabStateFlusher.flush(browser);

  return {tab, r};
}

function isValueInClosedData(rval) {
  return ss.getClosedTabData(window).includes(rval);
}

function restoreClosedTabWithValue(rval) {
  let closedTabData = JSON.parse(ss.getClosedTabData(window));
  let index = closedTabData.findIndex(function (data) {
    return (data.state.extData && data.state.extData.foobar) == rval;
  });

  if (index == -1) {
    throw new Error("no closed tab found for given rval");
  }

  return ss.undoCloseTab(window, index);
}

function promiseNewLocationAndHistoryEntryReplaced(browser, snippet) {
  return ContentTask.spawn(browser, snippet, function* (snippet) {
    let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
    let shistory = webNavigation.sessionHistory;

    
    eval(snippet);

    return new Promise(resolve => {
      let listener = {
        OnHistoryReplaceEntry() {
          shistory.removeSHistoryListener(this);
          resolve();
        },

        QueryInterface: XPCOMUtils.generateQI([
          Ci.nsISHistoryListener,
          Ci.nsISupportsWeakReference
        ])
      };

      shistory.addSHistoryListener(listener);

      
      addEventListener("unload", function () {
        try {
          shistory.removeSHistoryListener(listener);
        } catch (e) {  }
      });
    });
  });
}

function promiseHistoryEntryReplacedNonRemote(browser) {
  let {listeners} = promiseHistoryEntryReplacedNonRemote;

  return new Promise(resolve => {
    let shistory = browser.webNavigation.sessionHistory;

    let listener = {
      OnHistoryReplaceEntry() {
        shistory.removeSHistoryListener(this);
        resolve();
      },

      QueryInterface: XPCOMUtils.generateQI([
        Ci.nsISHistoryListener,
        Ci.nsISupportsWeakReference
      ])
    };

    shistory.addSHistoryListener(listener);
    listeners.set(browser, listener);
  });
}
promiseHistoryEntryReplacedNonRemote.listeners = new WeakMap();

add_task(function* dont_save_empty_tabs() {
  let {tab, r} = yield createTabWithRandomValue("about:blank");

  
  let promise = promiseRemoveTab(tab);

  
  ok(!isValueInClosedData(r), "closed tab not saved");
  yield promise;

  
  ok(!isValueInClosedData(r), "closed tab not saved");
});

add_task(function* save_worthy_tabs_remote() {
  let {tab, r} = yield createTabWithRandomValue("https://example.com/");
  ok(tab.linkedBrowser.isRemoteBrowser, "browser is remote");

  
  let promise = promiseRemoveTab(tab);

  
  ok(isValueInClosedData(r), "closed tab saved");
  yield promise;

  
  ok(isValueInClosedData(r), "closed tab saved");
});

add_task(function* save_worthy_tabs_nonremote() {
  let {tab, r} = yield createTabWithRandomValue("about:robots");
  ok(!tab.linkedBrowser.isRemoteBrowser, "browser is not remote");

  
  let promise = promiseRemoveTab(tab);

  
  ok(isValueInClosedData(r), "closed tab saved");
  yield promise;

  
  ok(isValueInClosedData(r), "closed tab saved");
});

add_task(function* save_worthy_tabs_remote_final() {
  let {tab, r} = yield createTabWithRandomValue("about:blank");
  let browser = tab.linkedBrowser;
  ok(browser.isRemoteBrowser, "browser is remote");

  
  let snippet = 'webNavigation.loadURI("https://example.com/", null, null, null, null)';
  yield promiseNewLocationAndHistoryEntryReplaced(browser, snippet);

  
  ok(browser.isRemoteBrowser, "browser is still remote");

  
  let promise = promiseRemoveTab(tab);

  
  ok(!isValueInClosedData(r), "closed tab not saved");
  yield promise;

  
  ok(isValueInClosedData(r), "closed tab saved");
});

add_task(function* save_worthy_tabs_nonremote_final() {
  let {tab, r} = yield createTabWithRandomValue("about:blank");
  let browser = tab.linkedBrowser;
  ok(browser.isRemoteBrowser, "browser is remote");

  
  yield BrowserTestUtils.loadURI(browser, "about:robots");
  ok(!browser.isRemoteBrowser, "browser is not remote anymore");

  
  yield promiseHistoryEntryReplacedNonRemote(browser);

  
  let promise = promiseRemoveTab(tab);

  
  ok(!isValueInClosedData(r), "closed tab not saved");
  yield promise;

  
  ok(isValueInClosedData(r), "closed tab saved");
});

add_task(function* dont_save_empty_tabs_final() {
  let {tab, r} = yield createTabWithRandomValue("https://example.com/");
  let browser = tab.linkedBrowser;

  
  let snippet = 'content.location.replace("about:blank")';
  yield promiseNewLocationAndHistoryEntryReplaced(browser, snippet);

  
  let promise = promiseRemoveTab(tab);

  
  ok(isValueInClosedData(r), "closed tab saved");
  yield promise;

  
  ok(!isValueInClosedData(r), "closed tab not saved");
});

add_task(function* undo_worthy_tabs() {
  let {tab, r} = yield createTabWithRandomValue("https://example.com/");
  ok(tab.linkedBrowser.isRemoteBrowser, "browser is remote");

  
  let promise = promiseRemoveTab(tab);

  
  ok(isValueInClosedData(r), "closed tab saved");

  
  tab = restoreClosedTabWithValue(r);

  
  yield promise;

  
  ok(!isValueInClosedData(r), "tab no longer closed");

  
  yield promiseRemoveTab(tab);
});

add_task(function* forget_worthy_tabs_remote() {
  let {tab, r} = yield createTabWithRandomValue("https://example.com/");
  ok(tab.linkedBrowser.isRemoteBrowser, "browser is remote");

  
  let promise = promiseRemoveTab(tab);

  
  ok(isValueInClosedData(r), "closed tab saved");

  
  ss.forgetClosedTab(window, 0);

  
  yield promise;

  
  ok(!isValueInClosedData(r), "we forgot about the tab");
});
