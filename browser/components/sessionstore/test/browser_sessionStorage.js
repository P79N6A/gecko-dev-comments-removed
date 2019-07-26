



let Scope = {};
Cu.import("resource://gre/modules/Task.jsm", Scope);
Cu.import("resource://gre/modules/Promise.jsm", Scope);
let {Task, Promise} = Scope;

function promiseBrowserLoaded(aBrowser) {
  let deferred = Promise.defer();
  whenBrowserLoaded(aBrowser, () => deferred.resolve());
  return deferred.promise;
}

function forceWriteState() {
  let deferred = Promise.defer();
  const PREF = "browser.sessionstore.interval";
  const TOPIC = "sessionstore-state-write";

  Services.obs.addObserver(function observe() {
    Services.obs.removeObserver(observe, TOPIC);
    Services.prefs.clearUserPref(PREF);
    deferred.resolve();
  }, TOPIC, false);

  Services.prefs.setIntPref(PREF, 0);
  return deferred.promise;
}

function waitForStorageChange(aTab) {
  let deferred = Promise.defer();
  waitForContentMessage(aTab.linkedBrowser,
    "sessionstore:MozStorageChanged",
    200,
    ((x) => deferred.resolve(x)));
  return deferred.promise;
}

function test() {

  waitForExplicitFinish();

  let tab;
  Task.spawn(function() {
    try {
      tab = gBrowser.addTab("http://example.com");
      

      let win = tab.linkedBrowser.contentWindow;

      
      
      yield promiseBrowserLoaded(tab.linkedBrowser);
      yield forceWriteState();
      info("Calling getBrowserState() to populate cache");
      ss.getBrowserState();

      info("Change sessionStorage, ensure that state is saved");
      win.sessionStorage["SESSION_STORAGE_KEY"] = "SESSION_STORAGE_VALUE";
      yield waitForStorageChange(tab);
      yield forceWriteState();

      let state = ss.getBrowserState();
      ok(state.indexOf("SESSION_STORAGE_KEY") != -1, "Key appears in state");
      ok(state.indexOf("SESSION_STORAGE_VALUE") != -1, "Value appears in state");


      info("Change localStorage, ensure that state is not saved");
      win.localStorage["LOCAL_STORAGE_KEY"] = "LOCAL_STORAGE_VALUE";
      yield waitForStorageChange(tab);
      yield forceWriteState();

      state = ss.getBrowserState();
      ok(state.indexOf("LOCAL_STORAGE_KEY") == -1, "Key does not appear in state");
      ok(state.indexOf("LOCAL_STORAGE_VALUE") == -1, "Value does not appear in state");
    } catch (ex) {
      ok(false, ex);
      info(ex.stack);
    } finally {
      
      if (tab) {
        gBrowser.removeTab(tab);
      }

      executeSoon(finish);
    }
  });
}
