Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");

function whenDelayedStartupFinished(aWindow, aCallback) {
  Services.obs.addObserver(function observer(aSubject, aTopic) {
    if (aWindow == aSubject) {
      Services.obs.removeObserver(observer, aTopic);
      executeSoon(aCallback);
    }
  }, "browser-delayed-startup-finished", false);
}











function promiseTopicObserved(topic)
{
  let deferred = Promise.defer();
  info("Waiting for observer topic " + topic);
  Services.obs.addObserver(function PTO_observe(subject, topic, data) {
    Services.obs.removeObserver(PTO_observe, topic);
    deferred.resolve([subject, data]);
  }, topic, false);
  return deferred.promise;
}
















function promiseTabLoadEvent(tab, url, eventType="load")
{
  let deferred = Promise.defer();
  info("Wait tab event: " + eventType);

  function handle(event) {
    if (event.originalTarget != tab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank" ||
        (url && event.target.location.href != url)) {
      info("Skipping spurious '" + eventType + "'' event" +
           " for " + event.target.location.href);
      return;
    }
    clearTimeout(timeout);
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    info("Tab event received: " + eventType);
    deferred.resolve(event);
  }

  let timeout = setTimeout(() => {
    if (tab.linkedBrowser)
      tab.linkedBrowser.removeEventListener(eventType, handle, true);
    deferred.reject(new Error("Timed out while waiting for a '" + eventType + "'' event"));
  }, 30000);

  tab.linkedBrowser.addEventListener(eventType, handle, true, true);
  if (url)
    tab.linkedBrowser.loadURI(url);
  return deferred.promise;
}

const PREF_SECURITY_DELAY_INITIAL = Services.prefs.getIntPref("security.notification_enable_delay");

function setup() {
  
  

  registerCleanupFunction(() => {
    PopupNotifications.buttonDelay = PREF_SECURITY_DELAY_INITIAL;
  });
}

function goNext() {
  executeSoon(() => executeSoon(Task.async(runNextTest)));
}

function* runNextTest() {
  if (tests.length == 0) {
    executeSoon(finish);
    return;
  }

  let nextTest = tests.shift();
  if (nextTest.onShown) {
    let shownState = false;
    onPopupEvent("popupshowing", function () {
      info("[" + nextTest.id + "] popup showing");
    });
    onPopupEvent("popupshown", function () {
      shownState = true;
      info("[" + nextTest.id + "] popup shown");
      Task.spawn(() => nextTest.onShown(this))
          .then(undefined , ex => Assert.ok(false, "onShown failed: " + ex));
    });
    onPopupEvent("popuphidden", function () {
      info("[" + nextTest.id + "] popup hidden");
      nextTest.onHidden(this);
      goNext();
    }, () => shownState);
    info("[" + nextTest.id + "] added listeners; panel is open: " + PopupNotifications.isPanelOpen);
  }

  info("[" + nextTest.id + "] running test");
  yield nextTest.run();
}

function showNotification(notifyObj) {
  info("Showing notification " + notifyObj.id);
  return PopupNotifications.show(notifyObj.browser,
                                 notifyObj.id,
                                 notifyObj.message,
                                 notifyObj.anchorID,
                                 notifyObj.mainAction,
                                 notifyObj.secondaryActions,
                                 notifyObj.options);
}

function dismissNotification(popup) {
  info("Dismissing notification " + popup.childNodes[0].id);
  executeSoon(() => EventUtils.synthesizeKey("VK_ESCAPE", {}));
}

function BasicNotification(testId) {
  this.browser = gBrowser.selectedBrowser;
  this.id = "test-notification-" + testId;
  this.message = "This is popup notification for " + testId;
  this.anchorID = null;
  this.mainAction = {
    label: "Main Action",
    accessKey: "M",
    callback: () => this.mainActionClicked = true
  };
  this.secondaryActions = [
    {
      label: "Secondary Action",
      accessKey: "S",
      callback: () => this.secondaryActionClicked = true
    }
  ];
  this.options = {
    eventCallback: eventName => {
      switch (eventName) {
        case "dismissed":
          this.dismissalCallbackTriggered = true;
          break;
        case "showing":
          this.showingCallbackTriggered = true;
          break;
        case "shown":
          this.shownCallbackTriggered = true;
          break;
        case "removed":
          this.removedCallbackTriggered = true;
          break;
        case "swapping":
          this.swappingCallbackTriggered = true;
          break;
      }
    }
  };
}

BasicNotification.prototype.addOptions = function(options) {
  for (let [name, value] in Iterator(options))
    this.options[name] = value;
};

function ErrorNotification() {
  this.mainAction.callback = () => {
    this.mainActionClicked = true;
    throw new Error("Oops!");
  };
  this.secondaryActions[0].callback = () => {
    this.secondaryActionClicked = true;
    throw new Error("Oops!");
  };
}

ErrorNotification.prototype = new BasicNotification();
ErrorNotification.prototype.constructor = ErrorNotification;

function checkPopup(popup, notifyObj) {
  info("Checking notification " + notifyObj.id);

  ok(notifyObj.showingCallbackTriggered, "showing callback was triggered");
  ok(notifyObj.shownCallbackTriggered, "shown callback was triggered");

  let notifications = popup.childNodes;
  is(notifications.length, 1, "one notification displayed");
  let notification = notifications[0];
  if (!notification)
    return;
  let icon = document.getAnonymousElementByAttribute(notification, "class",
                                                     "popup-notification-icon");
  if (notifyObj.id == "geolocation") {
    isnot(icon.boxObject.width, 0, "icon for geo displayed");
    is(popup.anchorNode.className, "notification-anchor-icon",
       "notification anchored to icon");
  }
  is(notification.getAttribute("label"), notifyObj.message, "message matches");
  is(notification.id, notifyObj.id + "-notification", "id matches");
  if (notifyObj.mainAction) {
    is(notification.getAttribute("buttonlabel"), notifyObj.mainAction.label,
       "main action label matches");
    is(notification.getAttribute("buttonaccesskey"),
       notifyObj.mainAction.accessKey, "main action accesskey matches");
  }
  let actualSecondaryActions =
    Array.filter(notification.childNodes, child => child.nodeName == "menuitem");
  let secondaryActions = notifyObj.secondaryActions || [];
  let actualSecondaryActionsCount = actualSecondaryActions.length;
  if (notifyObj.options.hideNotNow) {
    is(notification.getAttribute("hidenotnow"), "true", "'Not Now' item hidden");
    if (secondaryActions.length)
      is(notification.lastChild.tagName, "menuitem", "no menuseparator");
  }
  else if (secondaryActions.length) {
    is(notification.lastChild.tagName, "menuseparator", "menuseparator exists");
  }
  is(actualSecondaryActionsCount, secondaryActions.length,
    actualSecondaryActions.length + " secondary actions");
  secondaryActions.forEach(function (a, i) {
    is(actualSecondaryActions[i].getAttribute("label"), a.label,
       "label for secondary action " + i + " matches");
    is(actualSecondaryActions[i].getAttribute("accesskey"), a.accessKey,
       "accessKey for secondary action " + i + " matches");
  });
}

XPCOMUtils.defineLazyGetter(this, "gActiveListeners", () => {
  let listeners = new Map();
  registerCleanupFunction(() => {
    for (let [listener, eventName] of listeners) {
      PopupNotifications.panel.removeEventListener(eventName, listener, false);
    }
  });
  return listeners;
});

function onPopupEvent(eventName, callback, condition) {
  let listener = event => {
    if (event.target != PopupNotifications.panel ||
        (condition && !condition()))
      return;
    PopupNotifications.panel.removeEventListener(eventName, listener, false);
    gActiveListeners.delete(listener);
    executeSoon(() => callback.call(PopupNotifications.panel));
  }
  gActiveListeners.set(listener, eventName);
  PopupNotifications.panel.addEventListener(eventName, listener, false);
}

function triggerMainCommand(popup) {
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];
  info("Triggering main command for notification " + notification.id);
  
  EventUtils.synthesizeMouse(notification.button, 20, 10, {});
}

function triggerSecondaryCommand(popup, index) {
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];
  info("Triggering secondary command for notification " + notification.id);
  
  
  document.getAnonymousNodes(popup)[0].style.transition = "none";

  notification.button.focus();

  popup.addEventListener("popupshown", function handle() {
    popup.removeEventListener("popupshown", handle, false);
    info("Command popup open for notification " + notification.id);
    
    for (let i = 0; i <= index; i++) {
      EventUtils.synthesizeKey("VK_DOWN", {});
    }
    
    EventUtils.synthesizeKey("VK_RETURN", {});
  }, false);

  
  info("Open the popup to trigger secondary command for notification " + notification.id);
  EventUtils.synthesizeKey("VK_DOWN", { altKey: !navigator.platform.includes("Mac") });
}
