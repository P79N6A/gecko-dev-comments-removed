




































function test() {
  waitForExplicitFinish();

  ok(PopupNotifications, "PopupNotifications object exists");
  ok(PopupNotifications.panel, "PopupNotifications panel exists");

  registerCleanupFunction(cleanUp);

  runNextTest();
}

function cleanUp() {
  for (var topic in gActiveObservers)
    Services.obs.removeObserver(gActiveObservers[topic], topic);
  for (var eventName in gActiveListeners)
    PopupNotifications.panel.removeEventListener(eventName, gActiveListeners[eventName], false);
}

var gActiveListeners = {};
var gActiveObservers = {};

function runNextTest() {
  let nextTest = tests[gTestIndex];

  function goNext() {
    if (++gTestIndex == tests.length)
      executeSoon(finish);
    else
      executeSoon(runNextTest);
  }

  function addObserver(topic) {
    function observer() {
      Services.obs.removeObserver(observer, "PopupNotifications-" + topic);
      delete gActiveObservers["PopupNotifications-" + topic];

      info("[Test #" + gTestIndex + "] observer for " + topic + " called");
      nextTest[topic]();
      goNext();
    }
    Services.obs.addObserver(observer, "PopupNotifications-" + topic, false);
    gActiveObservers["PopupNotifications-" + topic] = observer;
  }

  if (nextTest.backgroundShow) {
    addObserver("backgroundShow");
  } else if (nextTest.updateNotShowing) {
    addObserver("updateNotShowing");
  } else {
    doOnPopupEvent("popupshowing", function () {
      info("[Test #" + gTestIndex + "] popup showing");
    });
    doOnPopupEvent("popupshown", function () {
      info("[Test #" + gTestIndex + "] popup shown");
      nextTest.onShown(this);
    });
  
    doOnPopupEvent("popuphidden", function () {
      info("[Test #" + gTestIndex + "] popup hidden");
      nextTest.onHidden(this);
  
      goNext();
    });
    info("[Test #" + gTestIndex + "] added listeners; panel state: " + PopupNotifications.isPanelOpen);
  }

  info("[Test #" + gTestIndex + "] running test");
  nextTest.run();
}

function doOnPopupEvent(eventName, callback) {
  gActiveListeners[eventName] = function (event) {
    if (event.target != PopupNotifications.panel)
      return;
    PopupNotifications.panel.removeEventListener(eventName, gActiveListeners[eventName], false);
    delete gActiveListeners[eventName];

    callback.call(PopupNotifications.panel);
  }
  PopupNotifications.panel.addEventListener(eventName, gActiveListeners[eventName], false);
}

var gTestIndex = 0;
var gNewTab;

function basicNotification() {
  var self = this;
  this.browser = gBrowser.selectedBrowser;
  this.id = "test-notification";
  this.message = "This is popup notification " + this.id + " from test " + gTestIndex;
  this.anchorID = null;
  this.mainAction = {
    label: "Main Action",
    accessKey: "M",
    callback: function () {
      self.mainActionClicked = true;
    }
  };
  this.secondaryActions = [
    {
      label: "Secondary Action",
      accessKey: "S",
      callback: function () {
        self.secondaryActionClicked = true;
      }
    }
  ];
}

var wrongBrowserNotificationObject = new basicNotification();
var wrongBrowserNotification;

var tests = [
  { 
    run: function () {
      this.notifyObj = new basicNotification(),
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.mainActionClicked, "mainAction was clicked");
    }
  },
  { 
    run: function () {
      this.notifyObj = new basicNotification(),
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.secondaryActionClicked, "secondaryAction was clicked");
    }
  },
  { 
    run: function () {
      this.notifyObj = new basicNotification(),
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      this.notification.remove();
    }
  },
  
  { 
    run: function () {
      gNewTab = gBrowser.addTab("about:blank");
      isnot(gBrowser.selectedTab, gNewTab, "new tab isn't selected");
      wrongBrowserNotificationObject.browser = gBrowser.getBrowserForTab(gNewTab);
      wrongBrowserNotification = showNotification(wrongBrowserNotificationObject);
    },
    backgroundShow: function () {
      is(PopupNotifications.isPanelOpen, false, "panel isn't open");
      ok(!wrongBrowserNotificationObject.mainActionClicked, "main action wasn't clicked");
      ok(!wrongBrowserNotificationObject.secondaryActionClicked, "secondary action wasn't clicked");
    }
  },
  
  { 
    run: function () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gNewTab;
    },
    onShown: function (popup) {
      checkPopup(popup, wrongBrowserNotificationObject);
      is(PopupNotifications.isPanelOpen, true, "isPanelOpen getter doesn't lie");

      
      gBrowser.selectedTab = this.oldSelectedTab;

    },
    onHidden: function (popup) {
      
      wrongBrowserNotification.remove();
      wrongBrowserNotification = null;
    }
  },
  
  { 
    run: function () {
      gBrowser.selectedTab = gNewTab;
    },
    updateNotShowing: function () {
      is(PopupNotifications.isPanelOpen, false, "panel isn't open");
      gBrowser.removeTab(gNewTab);
    }
  },
  
  
  { 
    run: function () {
      this.notifyObj = new basicNotification(),
      
      this.notification1 = showNotification(this.notifyObj);
      this.notification2 = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
    }
  },
  
  { 
    run: function () {
      this.testNotif1 = new basicNotification();
      this.testNotif1.message += " 1";
      showNotification(this.testNotif1);
      this.testNotif2 = new basicNotification();
      this.testNotif2.message += " 2";
      this.testNotif2.id = "test-notification-2";
      showNotification(this.testNotif2);
    },
    onShown: function (popup) {
      is(popup.childNodes.length, 2, "two notifications are shown");
      
      
      
      triggerMainCommand(popup);
      is(popup.childNodes.length, 1, "only one notification left");
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.testNotif1.mainActionClicked, "main action #1 was clicked");
      ok(!this.testNotif1.secondaryActionClicked, "secondary action #1 wasn't clicked");

      ok(!this.testNotif2.mainActionClicked, "main action #2 wasn't clicked");
      ok(this.testNotif2.secondaryActionClicked, "secondary action #2 was clicked");
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification(),
      this.notifyObj.mainAction = null;
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.firstNotification = showNotification(this.notifyObj);
      this.notifyObj2 = new basicNotification();
      this.notifyObj2.id += "-2";
      this.notifyObj2.anchorID = "urlbar";
      
      this.secondNotification = showNotification(this.notifyObj2);
    },
    onShown: function (popup) {
      
      checkPopup(popup, this.notifyObj2);
      is(document.getElementById("geo-notification-icon").boxObject.width, 0,
         "geo anchor shouldn't be visible");
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      
      this.firstNotification.remove();
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.notifyObj.secondaryActions = undefined;
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      this.notification.remove();
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.notifyObj.anchorID = "geo-notification-icon";
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      isnot(document.getElementById("geo-notification-icon").boxObject.width, 0,
            "geo anchor should be visible");
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      this.notification.remove();
    }
  },
];

function showNotification(notifyObj) {
  return PopupNotifications.show(notifyObj.browser,
                                 notifyObj.id,
                                 notifyObj.message,
                                 notifyObj.anchorID,
                                 notifyObj.mainAction,
                                 notifyObj.secondaryActions);
}

function checkPopup(popup, notificationObj) {
  info("[Test #" + gTestIndex + "] checking popup");
  let notifications = popup.childNodes;

  is(notifications.length, 1, "only one notification displayed");
  let notification = notifications[0];
  is(notification.getAttribute("label"), notificationObj.message, "message matches");
  is(notification.id, notificationObj.id, "id matches");
  if (notificationObj.mainAction) {
    is(notification.getAttribute("buttonlabel"), notificationObj.mainAction.label, "main action label matches");
    is(notification.getAttribute("buttonaccesskey"), notificationObj.mainAction.accessKey, "main action accesskey matches");
  }
  let actualSecondaryActions = notification.childNodes;
  let secondaryActions = notificationObj.secondaryActions || [];
  is(actualSecondaryActions.length, secondaryActions.length, actualSecondaryActions.length + " secondary actions");
  secondaryActions.forEach(function (a, i) {
    is(actualSecondaryActions[i].getAttribute("label"), a.label, "label for secondary action " + i + " matches");
    is(actualSecondaryActions[i].getAttribute("accesskey"), a.accessKey, "accessKey for secondary action " + i + " matches");
  });
}

function triggerMainCommand(popup) {
  info("[Test #" + gTestIndex + "] triggering main command");
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];

  
  EventUtils.synthesizeMouse(notification.button, 20, 10, {});
}

function triggerSecondaryCommand(popup, index) {
  info("[Test #" + gTestIndex + "] triggering secondary command");
  let notifications = popup.childNodes;
  ok(notifications.length > 0, "at least one notification displayed");
  let notification = notifications[0];

  notification.button.focus();

  popup.addEventListener("popupshown", function () {
    popup.removeEventListener("popupshown", arguments.callee, false);

    
    for (let i = 0; i <= index; i++)
      EventUtils.synthesizeKey("VK_DOWN", {});

    
    EventUtils.synthesizeKey("VK_ENTER", {});
  }, false);

  
  EventUtils.synthesizeKey("VK_DOWN", { altKey: (navigator.platform.indexOf("Mac") == -1) });
}

function dismissNotification(popup) {
  info("[Test #" + gTestIndex + "] dismissing notification");
  executeSoon(function () {
    EventUtils.synthesizeKey("VK_ESCAPE", {});
  });
}
