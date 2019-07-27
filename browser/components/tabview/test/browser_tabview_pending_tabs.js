


"use strict";
Components.utils.import("resource://gre/modules/Promise.jsm", this);

const STATE = {
  windows: [{
    tabs: [{
      entries: [{ url: "about:mozilla" }],
      hidden: true,
      extData: {"tabview-tab": '{"url":"about:mozilla","groupID":1}'}
    },{
      entries: [{ url: "about:robots" }],
      hidden: false,
      extData: {"tabview-tab": '{"url":"about:robots","groupID":1}'},
    }],
    selected: 1,
    extData: {
      "tabview-groups": '{"nextID":2,"activeGroupId":1, "totalNumber":1}',
      "tabview-group":
        '{"1":{"bounds":{"left":15,"top":5,"width":280,"height":232},"id":1}}'
    }
  }]
};





add_task(function setup() {
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("browser.sessionstore.restore_on_demand");
  });
});




add_task(function () {
  
  let win = OpenBrowserWindow();
  yield promiseDelayedStartupFinished(win);

  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"]
             .getService(Ci.nsISessionStore)
             .setWindowState(win, JSON.stringify(STATE), true);

  
  yield promiseTabViewShown(win);

  let [tab1, tab2] = win.gBrowser.tabs;
  let cw = win.TabView.getContentWindow();

  
  
  
  cw.TabItems.update(tab2);
  cw.TabItems.update(tab1);

  let tabItem1 = tab1._tabViewTabItem;
  let tabItem2 = tab2._tabViewTabItem;

  
  
  yield promiseTabItemUpdated(tabItem1);

  
  ok(!tabItem1.isShowingCachedData(), "doesn't show cached data");
  ok(tabItem2.isShowingCachedData(), "shows cached data");

  
  yield promiseWindowClosed(win);
});

function promiseTabItemUpdated(tabItem) {
  let deferred = Promise.defer();

  tabItem.addSubscriber("updated", function onUpdated() {
    tabItem.removeSubscriber("updated", onUpdated);
    deferred.resolve();
  });

  return deferred.promise;
}

function promiseAllTabItemsUpdated(win) {
  let deferred = Promise.defer();
  afterAllTabItemsUpdated(deferred.resolve, win);
  return deferred.promise;
}

function promiseDelayedStartupFinished(win) {
  let deferred = Promise.defer();
  whenDelayedStartupFinished(win, deferred.resolve);
  return deferred.promise;
}

function promiseTabViewShown(win) {
  let deferred = Promise.defer();
  showTabView(deferred.resolve, win);
  return deferred.promise;
}

function promiseWindowClosed(win) {
  let deferred = Promise.defer();

  Services.obs.addObserver(function obs(subject, topic) {
    if (subject == win) {
      Services.obs.removeObserver(obs, topic);
      deferred.resolve();
    }
  }, "domwindowclosed", false);

  win.close();
  return deferred.promise;
}
