



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
      this.testNotif = new ErrorNotification();
      showNotification(this.testNotif);
    },
    onShown: function (popup) {
      checkPopup(popup, this.testNotif);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      ok(this.testNotif.mainActionClicked, "main action has been triggered");
    }
  },
  
  { id: "Test#2",
    run: function () {
      this.testNotif = new ErrorNotification();
      showNotification(this.testNotif);
    },
    onShown: function (popup) {
      checkPopup(popup, this.testNotif);
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.testNotif.secondaryActionClicked, "secondary action has been triggered");
    }
  },
  
  { id: "Test#3",
    run: function () {
      this.notifyObj1 = new BasicNotification(this.id);
      this.notifyObj1.id += "_1";
      this.notifyObj1.anchorID = "default-notification-icon";
      this.notification1 = showNotification(this.notifyObj1);
    },
    onShown: function (popup) {
      
      
      this.notifyObj2 = new BasicNotification(this.id);
      this.notifyObj2.id += "_2";
      this.notifyObj2.anchorID = "geo-notification-icon";
      this.notifyObj2.options.dismissed = true;
      this.notification2 = showNotification(this.notifyObj2);

      checkPopup(popup, this.notifyObj1);

      
      is(document.getElementById("default-notification-icon").getAttribute("showing"), "true",
         "notification1 anchor should be visible");
      is(document.getElementById("geo-notification-icon").getAttribute("showing"), "true",
         "notification2 anchor should be visible");

      dismissNotification(popup);
    },
    onHidden: function(popup) {
      this.notification1.remove();
      this.notification2.remove();
    }
  },
  
  { id: "Test#4",
    run: function() {
      this.notifyObj = new BasicNotification(this.id);
      let normalCallback = this.notifyObj.options.eventCallback;
      this.notifyObj.options.eventCallback = function (eventName) {
        if (eventName == "showing") {
          this.mainAction.label = "Alternate Label";
        }
        normalCallback.call(this, eventName);
      };
      showNotification(this.notifyObj);
    },
    onShown: function(popup) {
      
      
      
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function() { }
  },
  
  { id: "Test#5",
    run: function() {
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      let notifyObj = new BasicNotification(this.id);
      showNotification(notifyObj);
      let win = gBrowser.replaceTabWithWindow(gBrowser.selectedTab);
      whenDelayedStartupFinished(win, function() {
        let [tab] = win.gBrowser.tabs;
        let anchor = win.document.getElementById("default-notification-icon");
        win.PopupNotifications._reshowNotifications(anchor);
        ok(win.PopupNotifications.panel.childNodes.length == 0,
           "no notification displayed in new window");
        ok(notifyObj.swappingCallbackTriggered, "the swapping callback was triggered");
        ok(notifyObj.removedCallbackTriggered, "the removed callback was triggered");
        win.close();
        goNext();
      });
    }
  },
  
  { id: "Test#6",
    run: function() {
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      let notifyObj = new BasicNotification(this.id);
      let originalCallback = notifyObj.options.eventCallback;
      notifyObj.options.eventCallback = function (eventName) {
        originalCallback(eventName);
        return eventName == "swapping";
      };

      showNotification(notifyObj);
      let win = gBrowser.replaceTabWithWindow(gBrowser.selectedTab);
      whenDelayedStartupFinished(win, function() {
        let [tab] = win.gBrowser.tabs;
        let anchor = win.document.getElementById("default-notification-icon");
        win.PopupNotifications._reshowNotifications(anchor);
        checkPopup(win.PopupNotifications.panel, notifyObj);
        ok(notifyObj.swappingCallbackTriggered, "the swapping callback was triggered");
        win.close();
        goNext();
      });
    }
  },
  
  { id: "Test#7",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.options.hideNotNow = true;
      this.notifyObj.mainAction.dismiss = true;
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      this.notification.remove();
    }
  },
  
  { id: "Test#8",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.mainAction.dismiss = true;
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback was triggered");
      ok(!this.notifyObj.removedCallbackTriggered, "removed callback wasn't triggered");
      this.notification.remove();
    }
  },
  
  { id: "Test#9",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.secondaryActions[0].dismiss = true;
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback was triggered");
      ok(!this.notifyObj.removedCallbackTriggered, "removed callback wasn't triggered");
      this.notification.remove();
    }
  },
  
  { id: "Test#10",
    run: function() {
      let notifyObj = new BasicNotification(this.id);
      let originalCallback = notifyObj.options.eventCallback;
      notifyObj.options.eventCallback = function (eventName) {
        originalCallback(eventName);
        return eventName == "showing";
      };

      let notification = showNotification(notifyObj);
      ok(notifyObj.showingCallbackTriggered, "the showing callback was triggered");
      ok(!notifyObj.shownCallbackTriggered, "the shown callback wasn't triggered");
      notification.remove();
      goNext();
    }
  },
  
  { id: "Test#11",
    run: function() {
      this.notifyObj = new BasicNotification(this.id);
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);

      this.notifyObj.showingCallbackTriggered = false;
      this.notifyObj.shownCallbackTriggered = false;

      
      
      
      PopupNotifications._update();

      checkPopup(popup, this.notifyObj);

      this.notification.remove();
    },
    onHidden: function() { }
  },
  
  { id: "Test#12",
    run: function () {
      this.notifyObj1 = new BasicNotification(this.id);
      this.notifyObj1.id += "_1";
      this.notifyObj1.anchorID = "default-notification-icon";
      this.notifyObj1.options.dismissed = true;
      this.notification1 = showNotification(this.notifyObj1);

      this.notifyObj2 = new BasicNotification(this.id);
      this.notifyObj2.id += "_2";
      this.notifyObj2.anchorID = "geo-notification-icon";
      this.notifyObj2.options.dismissed = true;
      this.notification2 = showNotification(this.notifyObj2);

      this.notification2.dismissed = false;
      PopupNotifications._update();
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj2);
      this.notification1.remove();
      this.notification2.remove();
    },
    onHidden: function(popup) { }
  },
  
  { id: "Test#13",
    run: function() {
      let notifyObj = new BasicNotification(this.id);
      notifyObj.options.dismissed = true;
      let win = gBrowser.replaceTabWithWindow(gBrowser.addTab("about:blank"));
      whenDelayedStartupFinished(win, function() {
        showNotification(notifyObj);
        let anchor = document.getElementById("default-notification-icon");
        is(anchor.getAttribute("showing"), "true", "the anchor is shown");
        win.close();
        goNext();
      });
    }
  }
];
