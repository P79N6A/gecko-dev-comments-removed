



let Scope = {};
Cu.import("resource://gre/modules/Task.jsm", Scope);
Cu.import("resource://gre/modules/Promise.jsm", Scope);
let {Task, Promise} = Scope;












const URI_TO_LOAD = "about:mozilla";

function waitForLoadStarted(aTab) {
  let deferred = Promise.defer();
  waitForContentMessage(aTab.linkedBrowser,
    "SessionStore:loadStart",
    1000,
    deferred.resolve);
  return deferred.promise;
}

function waitForTabLoaded(aTab) {
  let deferred = Promise.defer();
  whenBrowserLoaded(aTab.linkedBrowser, deferred.resolve);
  return deferred.promise;
}

function waitForTabClosed() {
  let deferred = Promise.defer();
  let observer = function() {
    gBrowser.tabContainer.removeEventListener("TabClose", observer, true);
    deferred.resolve();
  };
  gBrowser.tabContainer.addEventListener("TabClose", observer, true);
  return deferred.promise;
}

function test() {
  waitForExplicitFinish();

  Task.spawn(function() {
    try {
      
      let tab = gBrowser.addTab("about:blank");
      yield waitForTabLoaded(tab);

      
      ss.getBrowserState();

      is(gBrowser.tabs[1], tab, "newly created tab should exist by now");
      ok(tab.linkedBrowser.__SS_data, "newly created tab should be in save state");

      
      tab.linkedBrowser.loadURI(URI_TO_LOAD);
      let loaded = yield waitForLoadStarted(tab);
      ok(loaded, "Load started");

      let tabClosing = waitForTabClosed();
      gBrowser.removeTab(tab);
      info("Now waiting for TabClose to close");
      yield tabClosing;

      
      tab = ss.undoCloseTab(window, 0);
      yield waitForTabLoaded(tab);
      is(tab.linkedBrowser.currentURI.spec, URI_TO_LOAD, "loading proceeded as expected");

      gBrowser.removeTab(tab);

      executeSoon(finish);
    } catch (ex) {
      ok(false, ex);
      info(ex.stack);
    }
  });
}
