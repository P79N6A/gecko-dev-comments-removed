


let {Log} = Cu.import("resource://gre/modules/Log.jsm", {});
let {Weave} = Cu.import("resource://services-sync/main.js", {});
let {Notifications} = Cu.import("resource://services-sync/notifications.js", {});

let {getInternalScheduler} = Cu.import("resource:///modules/readinglist/Scheduler.jsm", {});

let stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                   .getService(Ci.nsIStringBundleService)
                   .createBundle("chrome://weave/locale/services/sync.properties");


Log.repository.getLogger("browserwindow.syncui").addAppender(new Log.DumpAppender());

function promiseObserver(topic) {
  return new Promise(resolve => {
    let obs = (subject, topic, data) => {
      Services.obs.removeObserver(obs, topic);
      resolve(subject);
    }
    Services.obs.addObserver(obs, topic, false);
  });
}

add_task(function* prepare() {
  let xps = Components.classes["@mozilla.org/weave/service;1"]
                              .getService(Components.interfaces.nsISupports)
                              .wrappedJSObject;
  yield xps.whenLoaded();
  
  let oldNeedsSetup = window.gSyncUI._needsSetup;
  window.gSyncUI._needsSetup = () => false;
  registerCleanupFunction(() => {
    window.gSyncUI._needsSetup = oldNeedsSetup;
  });
});

add_task(function* testProlongedSyncError() {
  let promiseNotificationAdded = promiseObserver("weave:notification:added");
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  Weave.Status.sync = Weave.PROLONGED_SYNC_FAILURE;
  Weave.Status.login = Weave.LOGIN_SUCCEEDED;
  Services.obs.notifyObservers(null, "weave:ui:sync:error", null);

  let subject = yield promiseNotificationAdded;
  let notification = subject.wrappedJSObject.object; 
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.sync.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
  Weave.Status.sync = Weave.STATUS_OK;
  Services.obs.notifyObservers(null, "weave:ui:sync:finish", null);
  yield promiseNotificationRemoved;
  Assert.equal(Notifications.notifications.length, 0, "no notifications left");
});

add_task(function* testProlongedRLError() {
  let promiseNotificationAdded = promiseObserver("weave:notification:added");
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  let longAgo = new Date(Date.now() - 100 * 24 * 60 * 60 * 1000); 
  Services.prefs.setCharPref("readinglist.scheduler.lastSync", longAgo.toString());
  getInternalScheduler().state = ReadingListScheduler.STATE_ERROR_OTHER;
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:error", null);

  let subject = yield promiseNotificationAdded;
  let notification = subject.wrappedJSObject.object; 
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.sync.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
  Services.prefs.setCharPref("readinglist.scheduler.lastSync", Date.now().toString());
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
  yield promiseNotificationRemoved;
  Assert.equal(Notifications.notifications.length, 0, "no notifications left");
});

add_task(function* testSyncLoginError() {
  let promiseNotificationAdded = promiseObserver("weave:notification:added");
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  Weave.Status.sync = Weave.LOGIN_FAILED;
  Weave.Status.login = Weave.LOGIN_FAILED_LOGIN_REJECTED;
  Services.obs.notifyObservers(null, "weave:ui:sync:error", null);

  let subject = yield promiseNotificationAdded;
  let notification = subject.wrappedJSObject.object; 
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.login.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  Weave.Status.sync = Weave.STATUS_OK;
  Weave.Status.login = Weave.LOGIN_SUCCEEDED;
  let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
  Services.obs.notifyObservers(null, "weave:service:login:start", null);
  Services.obs.notifyObservers(null, "weave:service:login:finish", null);
  yield promiseNotificationRemoved;
  Assert.equal(Notifications.notifications.length, 0, "no notifications left");
});

add_task(function* testSyncLoginNetworkError() {
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  
  
  

  
  
  
  

  
  
  

  let sawNotificationAdded = false;
  let obs = (subject, topic, data) => {
    sawNotificationAdded = true;
  }
  Services.obs.addObserver(obs, "weave:notification:added", false);
  try {
    
    Weave.Status.sync = Weave.LOGIN_FAILED;
    Weave.Status.login = Weave.LOGIN_FAILED_LOGIN_REJECTED;
    Services.obs.notifyObservers(null, "weave:ui:login:error", null);
    Assert.ok(sawNotificationAdded);

    
    let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
    Services.obs.notifyObservers(null, "readinglist:sync:start", null);
    Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
    yield promiseNotificationRemoved;

    
    sawNotificationAdded = false;
    Weave.Status.sync = Weave.LOGIN_FAILED;
    Weave.Status.login = Weave.LOGIN_FAILED_NETWORK_ERROR;
    Services.obs.notifyObservers(null, "weave:ui:login:error", null);
    Assert.ok(!sawNotificationAdded);

    
    Weave.Status.sync = Weave.LOGIN_FAILED;
    Weave.Status.login = Weave.LOGIN_FAILED_SERVER_ERROR;
    Services.obs.notifyObservers(null, "weave:ui:login:error", null);
    Assert.ok(!sawNotificationAdded);
    
  } finally {
    Services.obs.removeObserver(obs, "weave:notification:added");
  }
});

add_task(function* testRLLoginError() {
  let promiseNotificationAdded = promiseObserver("weave:notification:added");
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  getInternalScheduler().state = ReadingListScheduler.STATE_ERROR_AUTHENTICATION;
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:error", null);

  let subject = yield promiseNotificationAdded;
  let notification = subject.wrappedJSObject.object; 
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.login.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  getInternalScheduler().state = ReadingListScheduler.STATE_OK;
  let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
  yield promiseNotificationRemoved;
  Assert.equal(Notifications.notifications.length, 0, "no notifications left");
});





add_task(function* testRLLoginErrorRemains() {
  let promiseNotificationAdded = promiseObserver("weave:notification:added");
  Assert.equal(Notifications.notifications.length, 0, "start with no notifications");

  
  getInternalScheduler().state = ReadingListScheduler.STATE_ERROR_AUTHENTICATION;
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:error", null);

  let subject = yield promiseNotificationAdded;
  let notification = subject.wrappedJSObject.object; 
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.login.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  promiseNotificationAdded = promiseObserver("weave:notification:added");
  Weave.Status.sync = Weave.PROLONGED_SYNC_FAILURE;
  Weave.Status.login = Weave.LOGIN_FAILED_LOGIN_REJECTED;
  Services.obs.notifyObservers(null, "weave:ui:sync:error", null);
  subject = yield promiseNotificationAdded;
  
  notification = subject.wrappedJSObject.object;
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.login.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  promiseNotificationAdded = promiseObserver("weave:notification:added");
  Weave.Status.sync = Weave.STATUS_OK;
  Weave.Status.login = Weave.LOGIN_SUCCEEDED;
  Services.obs.notifyObservers(null, "weave:ui:sync:finish", null);

  
  subject = yield promiseNotificationAdded;
  
  notification = subject.wrappedJSObject.object;
  Assert.equal(notification.title, stringBundle.GetStringFromName("error.login.title"));
  Assert.equal(Notifications.notifications.length, 1, "exactly 1 notification");

  
  getInternalScheduler().state = ReadingListScheduler.STATE_OK;
  let promiseNotificationRemoved = promiseObserver("weave:notification:removed");
  Services.obs.notifyObservers(null, "readinglist:sync:start", null);
  Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
  yield promiseNotificationRemoved;
  Assert.equal(Notifications.notifications.length, 0, "no notifications left");
});

function checkButtonsStatus(shouldBeActive) {
  let button = document.getElementById("sync-button");
  let panelbutton = document.getElementById("PanelUI-fxa-status");
  if (shouldBeActive) {
    Assert.equal(button.getAttribute("status"), "active");
    Assert.equal(panelbutton.getAttribute("syncstatus"), "active");
  } else {
    Assert.ok(!button.hasAttribute("status"));
    Assert.ok(!panelbutton.hasAttribute("syncstatus"));
  }
}

function testButtonActions(startNotification, endNotification) {
  checkButtonsStatus(false);
  
  Services.obs.notifyObservers(null, startNotification, null);
  checkButtonsStatus(true);
  
  Services.obs.notifyObservers(null, endNotification, null);
  checkButtonsStatus(false);
}

add_task(function* testButtonActivities() {
  
  CustomizableUI.addWidgetToArea("sync-button", CustomizableUI.AREA_PANEL);
  
  yield PanelUI.show();
  try {
    testButtonActions("weave:service:login:start", "weave:service:login:finish");
    testButtonActions("weave:service:login:start", "weave:service:login:error");

    testButtonActions("weave:service:sync:start", "weave:service:sync:finish");
    testButtonActions("weave:service:sync:start", "weave:service:sync:error");

    testButtonActions("readinglist:sync:start", "readinglist:sync:finish");
    testButtonActions("readinglist:sync:start", "readinglist:sync:error");

    
    Services.obs.notifyObservers(null, "weave:service:sync:start", null);
    checkButtonsStatus(true);
    Services.obs.notifyObservers(null, "readinglist:sync:start", null);
    checkButtonsStatus(true);
    Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
    
    checkButtonsStatus(true);
    
    Services.obs.notifyObservers(null, "readinglist:sync:start", null);
    checkButtonsStatus(true);
    
    Services.obs.notifyObservers(null, "weave:service:sync:finish", null);
    
    checkButtonsStatus(true);
    
    Services.obs.notifyObservers(null, "readinglist:sync:error", null);
    checkButtonsStatus(false);
  } finally {
    PanelUI.hide();
    CustomizableUI.removeWidgetFromArea("sync-button");
  }
});
