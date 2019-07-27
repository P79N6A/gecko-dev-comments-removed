




let wrongBrowserNotificationObject = new BasicNotification("wrongBrowser");
let wrongBrowserNotification;

function test() {
  waitForExplicitFinish();

  ok(PopupNotifications, "PopupNotifications object exists");
  ok(PopupNotifications.panel, "PopupNotifications panel exists");

  setup();
  goNext();
}

let tests = [
  { id: "Test#1",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.mainActionClicked, "mainAction was clicked");
      ok(!this.notifyObj.dismissalCallbackTriggered, "dismissal callback wasn't triggered");
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  { id: "Test#2",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.secondaryActionClicked, "secondaryAction was clicked");
      ok(!this.notifyObj.dismissalCallbackTriggered, "dismissal callback wasn't triggered");
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  { id: "Test#3",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback triggered");
      this.notification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  
  { id: "Test#4",
    run: function* () {
      let tab = gBrowser.addTab("about:blank");
      isnot(gBrowser.selectedTab, tab, "new tab isn't selected");
      wrongBrowserNotificationObject.browser = gBrowser.getBrowserForTab(tab);
      let promiseTopic = promiseTopicObserved("PopupNotifications-backgroundShow");
      wrongBrowserNotification = showNotification(wrongBrowserNotificationObject);
      yield promiseTopic;
      is(PopupNotifications.isPanelOpen, false, "panel isn't open");
      ok(!wrongBrowserNotificationObject.mainActionClicked, "main action wasn't clicked");
      ok(!wrongBrowserNotificationObject.secondaryActionClicked, "secondary action wasn't clicked");
      ok(!wrongBrowserNotificationObject.dismissalCallbackTriggered, "dismissal callback wasn't called");
      goNext();
    }
  },
  
  { id: "Test#5",
    run: function () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.tabs[gBrowser.tabs.length - 1];
    },
    onShown: function (popup) {
      checkPopup(popup, wrongBrowserNotificationObject);
      is(PopupNotifications.isPanelOpen, true, "isPanelOpen getter doesn't lie");

      
      gBrowser.selectedTab = this.oldSelectedTab;
    },
    onHidden: function (popup) {
      
      ok(wrongBrowserNotificationObject.dismissalCallbackTriggered, "dismissal callback triggered due to tab switch");
      wrongBrowserNotification.remove();
      ok(wrongBrowserNotificationObject.removedCallbackTriggered, "removed callback triggered");
      wrongBrowserNotification = null;
    }
  },
  
  { id: "Test#6",
    run: function* () {
      let promiseTopic = promiseTopicObserved("PopupNotifications-updateNotShowing");
      gBrowser.selectedTab = gBrowser.tabs[gBrowser.tabs.length - 1];
      yield promiseTopic;
      is(PopupNotifications.isPanelOpen, false, "panel isn't open");
      gBrowser.removeTab(gBrowser.selectedTab);
      goNext();
    }
  },
  
  
  { id: "Test#7",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      
      this.notification1 = showNotification(this.notifyObj);
      this.notification2 = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      this.notification2.remove();
    },
    onHidden: function (popup) {
      ok(!this.notifyObj.dismissalCallbackTriggered, "dismissal callback wasn't triggered");
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { id: "Test#8",
    run: function () {
      this.testNotif1 = new BasicNotification(this.id);
      this.testNotif1.message += " 1";
      showNotification(this.testNotif1);
      this.testNotif2 = new BasicNotification(this.id);
      this.testNotif2.message += " 2";
      this.testNotif2.id += "-2";
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
      ok(!this.testNotif1.dismissalCallbackTriggered, "dismissal callback #1 wasn't called");

      ok(!this.testNotif2.mainActionClicked, "main action #2 wasn't clicked");
      ok(this.testNotif2.secondaryActionClicked, "secondary action #2 was clicked");
      ok(!this.testNotif2.dismissalCallbackTriggered, "dismissal callback #2 wasn't called");
    }
  },
  
  { id: "Test#9",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.mainAction = null;
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
  
  { id: "Test#10",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.firstNotification = showNotification(this.notifyObj);
      this.notifyObj2 = new BasicNotification(this.id);
      this.notifyObj2.id += "-2";
      this.notifyObj2.anchorID = "addons-notification-icon";
      
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
      this.secondNotification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
      ok(this.notifyObj2.removedCallbackTriggered, "removed callback triggered");
    }
  }
];
