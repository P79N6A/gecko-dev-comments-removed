



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
  PopupNotifications.buttonDelay = PREF_SECURITY_DELAY_INITIAL;
}

const PREF_SECURITY_DELAY_INITIAL = Services.prefs.getIntPref("security.notification_enable_delay");

var gActiveListeners = {};
var gActiveObservers = {};
var gShownState = {};

function goNext() {
  if (++gTestIndex == tests.length)
    executeSoon(finish);
  else
    executeSoon(runNextTest);
}

function runNextTest() {
  let nextTest = tests[gTestIndex];

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
  } else if (nextTest.onShown) {
    doOnPopupEvent("popupshowing", function () {
      info("[Test #" + gTestIndex + "] popup showing");
    });
    doOnPopupEvent("popupshown", function () {
      gShownState[gTestIndex] = true;
      info("[Test #" + gTestIndex + "] popup shown");
      nextTest.onShown(this);
    });

    
    
    let onHiddenArray = nextTest.onHidden instanceof Array ?
                        nextTest.onHidden :
                        [nextTest.onHidden];
    doOnPopupEvent("popuphidden", function () {
      if (!gShownState[gTestIndex]) {
        
        info("Popup from test " + gTestIndex + " was hidden before its popupshown fired");
      }

      let onHidden = onHiddenArray.shift();
      info("[Test #" + gTestIndex + "] popup hidden (" + onHiddenArray.length + " hides remaining)");
      executeSoon(function () {
        onHidden.call(nextTest, this);
        if (!onHiddenArray.length)
          goNext();
      }.bind(this));
    }, onHiddenArray.length);
    info("[Test #" + gTestIndex + "] added listeners; panel state: " + PopupNotifications.isPanelOpen);
  }

  info("[Test #" + gTestIndex + "] running test");
  nextTest.run();
}

function doOnPopupEvent(eventName, callback, numExpected) {
  gActiveListeners[eventName] = function (event) {
    if (event.target != PopupNotifications.panel)
      return;
    if (typeof(numExpected) === "number")
      numExpected--;
    if (!numExpected) {
      PopupNotifications.panel.removeEventListener(eventName, gActiveListeners[eventName], false);
      delete gActiveListeners[eventName];
    }

    callback.call(PopupNotifications.panel);
  }
  PopupNotifications.panel.addEventListener(eventName, gActiveListeners[eventName], false);
}

var gTestIndex = 0;
var gNewTab;

function basicNotification() {
  var self = this;
  this.browser = gBrowser.selectedBrowser;
  this.id = "test-notification-" + gTestIndex;
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
  this.options = {
    eventCallback: function (eventName) {
      switch (eventName) {
        case "dismissed":
          self.dismissalCallbackTriggered = true;
          break;
        case "showing":
          self.showingCallbackTriggered = true;
          break;
        case "shown":
          self.shownCallbackTriggered = true;
          break;
        case "removed":
          self.removedCallbackTriggered = true;
          break;
        case "swapping":
          self.swappingCallbackTriggered = true;
          break;
      }
    }
  };
}

basicNotification.prototype.addOptions = function(options) {
  for (let [name, value] in Iterator(options))
    this.options[name] = value;
};

function errorNotification() {
  var self = this;
  this.mainAction.callback = function () {
    self.mainActionClicked = true;
    throw new Error("Oops!");
  };
  this.secondaryActions[0].callback = function () {
    self.secondaryActionClicked = true;
    throw new Error("Oops!");
  };
}

errorNotification.prototype = new basicNotification();
errorNotification.prototype.constructor = errorNotification;

var wrongBrowserNotificationObject = new basicNotification();
var wrongBrowserNotification;

var tests = [
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
      ok(!wrongBrowserNotificationObject.dismissalCallbackTriggered, "dismissal callback wasn't called");
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
      
      ok(wrongBrowserNotificationObject.dismissalCallbackTriggered, "dismissal callback triggered due to tab switch");
      wrongBrowserNotification.remove();
      ok(wrongBrowserNotificationObject.removedCallbackTriggered, "removed callback triggered");
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
      this.notifyObj = new basicNotification();
      
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
  
  { 
    run: function () {
      this.testNotif1 = new basicNotification();
      this.testNotif1.message += " 1";
      showNotification(this.testNotif1);
      this.testNotif2 = new basicNotification();
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
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.firstNotification = showNotification(this.notifyObj);
      this.notifyObj2 = new basicNotification();
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
    onHidden: [
      
      function (popup) {},
      function (popup) {
        
        this.firstNotification.remove();
        this.secondNotification.remove();
        ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
        ok(this.notifyObj2.removedCallbackTriggered, "removed callback triggered");
      }
    ]
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
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback triggered");
      this.notification.remove();
      ok(this.notifyObj.removedCallbackTriggered, "removed callback triggered");
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");

      let self = this;
      loadURI("http://example.com/", function() {
        self.notifyObj = new basicNotification();
        self.notifyObj.addOptions({
          persistence: 2
        });
        self.notification = showNotification(self.notifyObj);
      });
    },
    onShown: function (popup) {
      this.complete = false;

      let self = this;
      loadURI("http://example.org/", function() {
        loadURI("http://example.com/", function() {

          
          self.complete = true;

          loadURI("http://example.org/");
        });
      });
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after 3 page loads");
      ok(this.notifyObj.removedCallbackTriggered, "removal callback triggered");
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },
  
  { 
    run: function () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");

      let self = this;
      loadURI("http://example.com/", function() {
        self.notifyObj = new basicNotification();
        
        self.notifyObj.addOptions({
          timeout: Date.now() + 600000
        });
        self.notification = showNotification(self.notifyObj);
      });
    },
    onShown: function (popup) {
      this.complete = false;

      let self = this;
      loadURI("http://example.org/", function() {
        loadURI("http://example.com/", function() {

          
          self.notification.options.timeout = Date.now() - 1;
          self.complete = true;

          loadURI("http://example.org/");
        });
      });
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after the timeout was passed");
      this.notification.remove();
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },
  
  
  { 
    run: function () {
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");

      let self = this;
      loadURI("http://example.com/", function() {
        self.notifyObj = new basicNotification();
        self.notifyObj.addOptions({
          persistWhileVisible: true
        });
        self.notification = showNotification(self.notifyObj);
      });
    },
    onShown: function (popup) {
      this.complete = false;

      let self = this;
      loadURI("http://example.org/", function() {
        loadURI("http://example.com/", function() {

          
          self.complete = true;
          dismissNotification(popup);
        });
      });
    },
    onHidden: function (popup) {
      ok(this.complete, "Should only have hidden the notification after it was dismissed");
      this.notification.remove();
      gBrowser.removeTab(gBrowser.selectedTab);
      gBrowser.selectedTab = this.oldSelectedTab;
    }
  },
  
  { 
    run: function() {
      
      this.box = document.createElement("box");
      PopupNotifications.iconBox.appendChild(this.box);

      let button = document.createElement("button");
      button.setAttribute("label", "Please click me!");
      this.box.appendChild(button);

      
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function() {
      let notifyObj = new basicNotification();
      notifyObj.anchorID = "geo-notification-icon";
      notifyObj.addOptions({neverShow: true});
      showNotification(notifyObj);
    },
    updateNotShowing: function() {
      isnot(document.getElementById("geo-notification-icon").boxObject.width, 0,
            "geo anchor should be visible");
    }
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function () {
      window.locationbar.visible = false;
      this.notifyObj = new basicNotification();
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
  },
  
  { 
    run: function () {
      this.notifyObj = new basicNotification();
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
  
  { 
    run: function () {
      this.notifyObj1 = new basicNotification();
      this.notifyObj1.id += "_1";
      this.notifyObj1.anchorID = "default-notification-icon";
      this.notification1 = showNotification(this.notifyObj1);

      this.notifyObj2 = new basicNotification();
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
    onHidden: [
      function (popup) {
      },
      function (popup) {
        this.notification1.remove();
        ok(this.notifyObj1.removedCallbackTriggered, "removed callback triggered");

        this.notification2.remove();
        ok(this.notifyObj2.removedCallbackTriggered, "removed callback triggered");
      }
    ]
  },
  
  { 
    run: function () {
      
      this.notifyObjOld = new basicNotification();
      this.notifyObjOld.anchorID = "default-notification-icon";
      this.notificationOld = showNotification(this.notifyObjOld);

      
      this.oldSelectedTab = gBrowser.selectedTab;
      gBrowser.selectedTab = gBrowser.addTab("about:blank");

      
      this.notifyObjNew = new basicNotification();
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
    onHidden: [
      function (popup) {
      },
      function (popup) {
        this.notificationNew.remove();
        gBrowser.removeTab(gBrowser.selectedTab);

        gBrowser.selectedTab = this.oldSelectedTab;
        this.notificationOld.remove();
      }
    ]
  },
  { 
    run: function () {
      
      PopupNotifications.buttonDelay = 100000;

      this.notifyObj = new basicNotification();
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
  { 
    run: function () {
      
      PopupNotifications.buttonDelay = 10;

      this.notifyObj = new basicNotification();
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
  { 
    run: function () {
      loadURI("http://example.com/", function() {
        let notifyObj = new basicNotification();
        notifyObj.options.eventCallback = function (eventName) {
          if (eventName == "removed") {
            ok(true, "Notification removed in background tab after reloading");
            executeSoon(function () {
              goNext();
            });
          }
        };
        showNotification(notifyObj);
        executeSoon(function () {
          gBrowser.selectedBrowser.reload();
        });
      });
    }
  },
  { 
    run: function () {
      let oldSelectedTab = gBrowser.selectedTab;
      let newTab = gBrowser.addTab("about:blank");
      gBrowser.selectedTab = newTab;

      loadURI("http://example.com/", function() {
        gBrowser.selectedTab = oldSelectedTab;
        let browser = gBrowser.getBrowserForTab(newTab);

        let notifyObj = new basicNotification();
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
      });
    }
  },
  { 
    run: function () {
      loadURI("http://example.com/", function () {
        let originalTab = gBrowser.selectedTab;
        let bgTab = gBrowser.addTab("about:blank");
        gBrowser.selectedTab = bgTab;
        loadURI("http://example.com/", function () {
          let anchor = document.createElement("box");
          anchor.id = "test26-anchor";
          anchor.className = "notification-anchor-icon";
          PopupNotifications.iconBox.appendChild(anchor);

          gBrowser.selectedTab = originalTab;

          let fgNotifyObj = new basicNotification();
          fgNotifyObj.anchorID = anchor.id;
          fgNotifyObj.options.dismissed = true;
          let fgNotification = showNotification(fgNotifyObj);

          let bgNotifyObj = new basicNotification();
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
        });
      });
    }
  },
  { 
    run: function () {
      loadURI("data:text/html;charset=utf8,<iframe id='iframe' src='http://example.com/'>", function () {
        this.notifyObj = new basicNotification();
        this.notifyObj.options.eventCallback = function (eventName) {
          if (eventName == "removed") {
            ok(false, "Test 28: Notification removed from browser when subframe navigated");
          }
        };
        showNotification(this.notifyObj);
      }.bind(this));
    },
    onShown: function (popup) {
      let self = this;
      let progressListener = {
        onLocationChange: function onLocationChange(aBrowser) {
          if (aBrowser != gBrowser.selectedBrowser) {
            return;
          }
          let notification = PopupNotifications.getNotification(self.notifyObj.id,
                                                                self.notifyObj.browser);
          ok(notification != null, "Test 28: Notification remained when subframe navigated");
          self.notifyObj.options.eventCallback = undefined;

          notification.remove();
          gBrowser.removeTabsProgressListener(progressListener);
        },
      };

      info("Test 28: Adding progress listener and performing navigation");
      gBrowser.addTabsProgressListener(progressListener);
      content.document.getElementById("iframe")
                      .setAttribute("src", "http://example.org/");
    },
    onHidden: function () {}
  },
  { 
    run: function () {
      let callbackCount = 0;
      this.testNotif1 = new basicNotification();
      this.testNotif1.message += " 1";
      showNotification(this.testNotif1);
      this.testNotif1.options.eventCallback = function (eventName) {
        info("notifyObj1.options.eventCallback: " + eventName);
        if (eventName == "dismissed") {
          throw new Error("Oops 1!");
          if (++callbackCount == 2) {
            executeSoon(goNext);
          }
        }
      };

      this.testNotif2 = new basicNotification();
      this.testNotif2.message += " 2";
      this.testNotif2.id += "-2";
      this.testNotif2.options.eventCallback = function (eventName) {
        info("notifyObj2.options.eventCallback: " + eventName);
        if (eventName == "dismissed") {
          throw new Error("Oops 2!");
          if (++callbackCount == 2) {
            executeSoon(goNext);
          }
        }
      };
      showNotification(this.testNotif2);
    },
    onShown: function (popup) {
      is(popup.childNodes.length, 2, "two notifications are shown");
      dismissNotification(popup);
    },
    onHidden: function () {}
  },
  { 
    run: function () {
      this.testNotif = new errorNotification();
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
  { 
    run: function () {
      this.testNotif = new errorNotification();
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
  { 
    run: function () {
      this.notifyObj1 = new basicNotification();
      this.notifyObj1.id += "_1";
      this.notifyObj1.anchorID = "default-notification-icon";
      this.notification1 = showNotification(this.notifyObj1);
    },
    onShown: function (popup) {
      
      
      this.notifyObj2 = new basicNotification();
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
  { 
    run: function() {
      this.notifyObj = new basicNotification();
      var normalCallback = this.notifyObj.options.eventCallback;
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
  { 
    
    run: function() {
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      let notifyObj = new basicNotification();
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
  { 
    run: function() {
      gBrowser.selectedTab = gBrowser.addTab("about:blank");
      let notifyObj = new basicNotification();
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
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.notifyObj.options.hideNotNow = true;
      this.notifyObj.mainAction.dismiss = true;
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) { }
  },
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.notifyObj.mainAction.dismiss = true;
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerMainCommand(popup);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback was triggered");
      ok(!this.notifyObj.removedCallbackTriggered, "removed callback wasn't triggered");
    }
  },
  { 
    run: function () {
      this.notifyObj = new basicNotification();
      this.notifyObj.secondaryActions[0].dismiss = true;
      showNotification(this.notifyObj);
    },
    onShown: function (popup) {
      checkPopup(popup, this.notifyObj);
      triggerSecondaryCommand(popup, 0);
    },
    onHidden: function (popup) {
      ok(this.notifyObj.dismissalCallbackTriggered, "dismissal callback was triggered");
      ok(!this.notifyObj.removedCallbackTriggered, "removed callback wasn't triggered");
    }
  }
];

function showNotification(notifyObj) {
  return PopupNotifications.show(notifyObj.browser,
                                 notifyObj.id,
                                 notifyObj.message,
                                 notifyObj.anchorID,
                                 notifyObj.mainAction,
                                 notifyObj.secondaryActions,
                                 notifyObj.options);
}

function checkPopup(popup, notificationObj) {
  info("[Test #" + gTestIndex + "] checking popup");

  ok(notificationObj.showingCallbackTriggered, "showing callback was triggered");
  ok(notificationObj.shownCallbackTriggered, "shown callback was triggered");

  let notifications = popup.childNodes;
  is(notifications.length, 1, "one notification displayed");
  let notification = notifications[0];
  if (!notification)
    return;
  let icon = document.getAnonymousElementByAttribute(notification, "class", "popup-notification-icon");
  if (notificationObj.id == "geolocation") {
    isnot(icon.boxObject.width, 0, "icon for geo displayed");
    is(popup.anchorNode.className, "notification-anchor-icon", "notification anchored to icon");
  }
  is(notification.getAttribute("label"), notificationObj.message, "message matches");
  is(notification.id, notificationObj.id + "-notification", "id matches");
  if (notificationObj.mainAction) {
    is(notification.getAttribute("buttonlabel"), notificationObj.mainAction.label, "main action label matches");
    is(notification.getAttribute("buttonaccesskey"), notificationObj.mainAction.accessKey, "main action accesskey matches");
  }
  let actualSecondaryActions = Array.filter(notification.childNodes,
                                            function (child) child.nodeName == "menuitem");
  let secondaryActions = notificationObj.secondaryActions || [];
  let actualSecondaryActionsCount = actualSecondaryActions.length;
  if (notificationObj.options.hideNotNow) {
    is(notification.getAttribute("hidenotnow"), "true", "Not Now item hidden");
    if (secondaryActions.length)
      is(notification.lastChild.tagName, "menuitem", "no menuseparator");
  }
  else if (secondaryActions.length) {
    is(notification.lastChild.tagName, "menuseparator", "menuseparator exists");
  }
  is(actualSecondaryActionsCount, secondaryActions.length, actualSecondaryActions.length + " secondary actions");
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

  
  
  document.getAnonymousNodes(popup)[0].style.transition = "none";

  notification.button.focus();

  popup.addEventListener("popupshown", function () {
    popup.removeEventListener("popupshown", arguments.callee, false);

    
    for (let i = 0; i <= index; i++)
      EventUtils.synthesizeKey("VK_DOWN", {});

    
    EventUtils.synthesizeKey("VK_RETURN", {});
  }, false);

  
  EventUtils.synthesizeKey("VK_DOWN", { altKey: !navigator.platform.contains("Mac") });
}

function loadURI(uri, callback) {
  if (callback) {
    gBrowser.addEventListener("load", function() {
      
      if (gBrowser.currentURI.spec == "about:blank")
        return;

      gBrowser.removeEventListener("load", arguments.callee, true);

      callback();
    }, true);
  }
  gBrowser.loadURI(uri);
}

function dismissNotification(popup) {
  info("[Test #" + gTestIndex + "] dismissing notification");
  executeSoon(function () {
    EventUtils.synthesizeKey("VK_ESCAPE", {});
  });
}
