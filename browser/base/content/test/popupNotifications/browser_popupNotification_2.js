



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
      this.notifyObj.secondaryActions = undefined;
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
  
  { id: "Test#2",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.id = "geolocation";
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
      let icon = document.getElementById("geo-notification-icon");
      isnot(icon.boxObject.width, 0,
            "geo anchor should be visible after dismissal");
      this.notification.remove();
      is(icon.boxObject.width, 0,
         "geo anchor should not be visible after removal");
    }
  },

  
  { id: "Test#3",
    run: function* () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.addOptions({
        persistence: 2
      });
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function* (popup) {
      this.complete = false;
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.org/");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/")
      
      this.complete = true;
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.org/");
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after 3 page loads");
      ok(this.notifyObj.removedCallbackTriggered, "removal callback triggered");
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },
  
  { id: "Test#4",
    run: function* () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      this.notifyObj = new BasicNotification(this.id);
      
      this.notifyObj.addOptions({
        timeout: Date.now() + 600000
      });
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function* (popup) {
      this.complete = false;
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.org/");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      
      this.notification.options.timeout = Date.now() - 1;
      this.complete = true;
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.org/");
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after the timeout was passed");
      this.notification.remove();
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },
  
  
  { id: "Test#5",
    run: function* () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.addOptions({
        persistWhileVisible: true
      });
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function* (popup) {
      this.complete = false;

      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.org/");
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      
      this.complete = true;
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after it was dismissed");
      this.notification.remove();
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },

  
  { id: "Test#6",
    run: function() {
      
      this.box = document.createElement("box");
      PopupNotifications.iconBox.appendChild(this.box);

      let button = document.createElement("button");
      button.setAttribute("label", "Please click me!");
      this.box.appendChild(button);

      
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.anchorID = this.box.id = "nested-box";
      this.notifyObj.addOptions({dismissed: true});
      this.notification = showNotification(this.notifyObj);

      
      
      
      
      
      EventUtils.synthesizeMouse(button, 4, 4, {});
    },
    onShown: function(popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function(popup) {
      this.notification.remove();
      this.box.parentNode.removeChild(this.box);
    }
  },
  
  { id: "Test#7",
    run: function* () {
      let notifyObj = new BasicNotification(this.id);
      notifyObj.anchorID = "geo-notification-icon";
      notifyObj.addOptions({neverShow: true});
      let promiseTopic = promiseTopicObserved("PopupNotifications-updateNotShowing");
      showNotification(notifyObj);
      yield promiseTopic;
      isnot(document.getElementById("geo-notification-icon").boxObject.width, 0,
            "geo anchor should be visible");
      goNext();
    }
  },
  
  { id: "Test#8",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerSecondaryCommand(popup, 1);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback triggered");
      this.notification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { id: "Test#9",
    run: function () {
      this.notifyObj = new BasicNotification(this.id);
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      let notification = popup.childNodes[0];
      EventUtils.synthesizeMouseAtCenter(notification.closebutton, {});
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback triggered");
      this.notification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { id: "Test#10",
    run: function () {
      window.locationbar.visible = false;
      this.notifyObj = new BasicNotification(this.id);
      this.notification = showNotification(this.notifyObj);
      window.locationbar.visible = true;
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      is(popup.anchorNode.className, "tabbrowser-tab", "notification anchored to tab");
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback triggered");
      this.notification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  }
];
