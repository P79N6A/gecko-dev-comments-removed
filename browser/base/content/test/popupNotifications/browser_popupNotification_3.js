



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
      this.notifyObj.addOptions({
        removeOnDismissal: true
      });
      this.notification = showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      dismissNotification(popup);
    },
    onHidden: function (popup) {
      ok(!this.notifyObj.dismissalCallbackTriggered, "dismissal callback wasn't triggered");
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { id: "Test#2",
    run: function () {
      this.notifyObj1 = new BasicNotification(this.id);
      this.notifyObj1.id += "_1";
      this.notifyObj1.anchorID = "default-notification-icon";
      this.notification1 = showNotification(this.notifyObj1);

      this.notifyObj2 = new BasicNotification(this.id);
      this.notifyObj2.id += "_2";
      this.notifyObj2.anchorID = "geo-notification-icon";
      this.notification2 = showNotification(this.notifyObj2);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj2);

      
      isnot(document.getElementById("default-notification-icon").boxObject.width, 0,
            "default anchor should be visible");
      
      isnot(document.getElementById("geo-notification-icon").boxObject.width, 0,
            "geo anchor should be visible");

      dismissNotification(popup);
    },
    onHidden: function (popup) {
      this.notification1.remove();
      ok(this.notifyObj1.removedCallbackTriggered, "removed callback triggered");

      this.notification2.remove();
      ok(this.notifyObj2.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { id: "Test#3",
    run: function () {
      
      this.notifyObjOld = new BasicNotification(this.id);
      this.notifyObjOld.anchorID = "default-notification-icon";
      this.notificationOld = showNotification(this.notifyObjOld);

      
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");

      
      this.notifyObjNew = new BasicNotification(this.id);
      this.notifyObjNew.anchorID = "geo-notification-icon";
      this.notificationNew = showNotification(this.notifyObjNew);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObjNew);

      
      is(document.getElementById("default-notification-icon").boxObject.width, 0,
         "default anchor shouldn't be visible");
      
      isnot(document.getElementById("geo-notification-icon").boxObject.width, 0,
            "geo anchor should be visible");

      dismissNotification(popup);
    },
    onHidden: function (popup) {
      this.notificationNew.remove();
      gBrowser.removeTab(gBrowser.selectedTab);

      gBrowser.selectedTab = this.oldSelectedTab;
      this.notificationOld.remove();
    }
  },
  
  { id: "Test#4",
    run: function () {
      
      PopupNotifications.buttonDelay = 100000;

      this.notifyObj = new BasicNotification(this.id);
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);

      
      executeSoon(function delayedDismissal() {
        dismissNotification(popup);
      });

    },
    onHidden: function (popup) {
      ok(!this.notifyObj.mainActionClicked, "mainAction was not clicked because it was too soon");
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback was triggered");
    }
  },
  
  { id: "Test#5",
    run: function () {
      
      PopupNotifications.buttonDelay = 10;

      this.notifyObj = new BasicNotification(this.id);
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);

      
      setTimeout(function delayedDismissal() {
        triggerMainCommand(popup);
      }, 500);

    },
    onHidden: function (popup) {
      ok(this.notifyObj.mainActionClicked, "mainAction was clicked after the delay");
      ok(!this.notifyObj.dismissalCallbackTriggered, "dismissal callback was not triggered");
      PopupNotifications.buttonDelay = PREF_SECURITY_DELAY_INITIAL;
    }
  },
  
  { id: "Test#6",
    run: function* () {
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      let notifyObj = new BasicNotification(this.id);
      notifyObj.options.eventCallback = function (eventName) {
        if (eventName == "removed") {
          ok(true, "Notification removed in background tab after reloading");
          goNext();
        }
      };
      showNotification(notifyObj);
      executeSoon(function () {
        gBrowser.selectedBrowser.reload();
      });
    }
  },
  
  { id: "Test#7",
    run: function* () {
      let oldSelectedTab = gBrowser.selectedTab;
      let newTab = gBrowser.addTab("about:blank");
      gBrowser.selectedTab = newTab;

      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      gBrowser.selectedTab = oldSelectedTab;
      let browser = gBrowser.getBrowserForTab(newTab);

      let notifyObj = new BasicNotification(this.id);
      notifyObj.browser = browser;
      notifyObj.options.eventCallback = function (eventName) {
        if (eventName == "removed") {
          ok(true, "Notification removed in background tab after reloading");
          executeSoon(function () {
            gBrowser.removeTab(newTab);
            goNext();
          });
        }
      };
      showNotification(notifyObj);
      executeSoon(function () {
        browser.reload();
      });
    }
  },
  
  { id: "Test#8",
    run: function* () {
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      let originalTab = gBrowser.selectedTab;
      let bgTab = gBrowser.addTab("about:blank");
      gBrowser.selectedTab = bgTab;
      yield promiseTabLoadEvent(gBrowser.selectedTab, "http://example.com/");
      let anchor = document.createElement("box");
      anchor.id = "test26-anchor";
      anchor.className = "notification-anchor-icon";
      PopupNotifications.iconBox.appendChild(anchor);

      gBrowser.selectedTab = originalTab;

      let fgNotifyObj = new BasicNotification(this.id);
      fgNotifyObj.anchorID = anchor.id;
      fgNotifyObj.options.dismissed = true;
      let fgNotification = showNotification(fgNotifyObj);

      let bgNotifyObj = new BasicNotification(this.id);
      bgNotifyObj.anchorID = anchor.id;
      bgNotifyObj.browser = gBrowser.getBrowserForTab(bgTab);
      
      let bgNotification = showNotification(bgNotifyObj);
      
      bgNotification = showNotification(bgNotifyObj);

      ok(fgNotification.id, "notification has id");
      is(fgNotification.id, bgNotification.id, "notification ids are the same");
      is(anchor.getAttribute("showing"), "true", "anchor still showing");

      fgNotification.remove();
      gBrowser.removeTab(bgTab);
      goNext();
    }
  },
  
  { id: "Test#9",
    run: function* () {
      yield promiseTabLoadEvent(gBrowser.selectedTab, "data:text/html;charset=utf8,<iframe%20id='iframe'%20src='http://example.com/'>");
      this.notifyObj = new BasicNotification(this.id);
      this.notifyObj.options.eventCallback = function (eventName) {
        if (eventName == "removed") {
          ok(false, "Notification removed from browser when subframe navigated");
        }
      };
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      let self = this;
      let progressListener = {
        onLocationChange: function onLocationChange() {
          gBrowser.removeProgressListener(progressListener);

	  executeSoon(() => {
            let notification = PopupNotifications.getNotification(self.notifyObj.id,
                                                                  self.notifyObj.browser);
            ok(notification != null, "Notification remained when subframe navigated");
            self.notifyObj.options.eventCallback = undefined;

            notification.remove();
	  });
        },
      };

      info("Adding progress listener and performing navigation");
      gBrowser.addProgressListener(progressListener);
      content.document.getElementById("iframe")
                      .setAttribute("src", "http://example.org/");
    },
    onHidden: function () {}
  },
  
  { id: "Test#10",
    run: function () {
      let callbackCount = 0;
      this.testNotif1 = new BasicNotification(this.id);
      this.testNotif1.message += " 1";
      this.notification1 = showNotification(this.testNotif1);
      this.testNotif1.options.eventCallback = function (eventName) {
        info("notifyObj1.options.eventCallback: " + eventName);
        if (eventName == "dismissed") {
          throw new Error("Oops 1!");
          if (++callbackCount == 2) {
            goNext();
          }
        }
      };

      this.testNotif2 = new BasicNotification(this.id);
      this.testNotif2.message += " 2";
      this.testNotif2.id += "-2";
      this.testNotif2.options.eventCallback = function (eventName) {
        info("notifyObj2.options.eventCallback: " + eventName);
        if (eventName == "dismissed") {
          throw new Error("Oops 2!");
          if (++callbackCount == 2) {
            goNext();
          }
        }
      };
      this.notification2 = showNotification(this.testNotif2);
    },
    onShown: function (popup) {
      is(popup.childNodes.length, 2, "two notifications are shown");
      dismissNotification(popup);
    },
    onHidden: function () {
      this.notification1.remove();
      this.notification2.remove();
    }
  }
];
