










"use strict";

this.EXPORTED_SYMBOLS = [
  "BrowserTestUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://testing-common/TestUtils.jsm");

Cc["@mozilla.org/globalmessagemanager;1"]
  .getService(Ci.nsIMessageListenerManager)
  .loadFrameScript(
    "chrome://mochikit/content/tests/BrowserTestUtils/content-utils.js", true);

this.BrowserTestUtils = {
  




















  withNewTab: Task.async(function* (options, taskFn) {
    let tab = yield BrowserTestUtils.openNewForegroundTab(options.gBrowser, options.url);
    yield taskFn(tab.linkedBrowser);
    options.gBrowser.removeTab(tab);
  }),

  














  openNewForegroundTab(tabbrowser, opening = "about:blank", aWaitForLoad = true) {
    let tab;
    let promises = [
      BrowserTestUtils.switchTab(tabbrowser, function () {
        if (typeof opening == "function") {
          opening();
          tab = tabbrowser.selectedTab;
        }
        else {
          tabbrowser.selectedTab = tab = tabbrowser.addTab(opening);
        }
      })
    ];

    if (aWaitForLoad) {
      promises.push(BrowserTestUtils.browserLoaded(tab.linkedBrowser));
    }

    return Promise.all(promises).then(() => tab);
  },

  











  switchTab(tabbrowser, tab) {
    let promise = new Promise(resolve => {
      tabbrowser.addEventListener("TabSwitchDone", function onSwitch() {
        tabbrowser.removeEventListener("TabSwitchDone", onSwitch);
        TestUtils.executeSoon(() => resolve(tabbrowser.selectedTab));
      });
    });

    if (typeof tab == "function") {
      tab();
    }
    else {
      tabbrowser.selectedTab = tab;
    }
    return promise;
  },

  
















  browserLoaded(browser, includeSubFrames=false) {
    return new Promise(resolve => {
      browser.messageManager.addMessageListener("browser-test-utils:loadEvent",
                                                 function onLoad(msg) {
        if (!msg.data.subframe || includeSubFrames) {
          browser.messageManager.removeMessageListener(
            "browser-test-utils:loadEvent", onLoad);
          resolve();
        }
      });
    });
  },

  




  domWindowOpened() {
    return new Promise(resolve => {
      function observer(subject, topic, data) {
        if (topic != "domwindowopened") { return; }

        Services.ww.unregisterNotification(observer);
        resolve(subject.QueryInterface(Ci.nsIDOMWindow));
      }
      Services.ww.registerNotification(observer);
    });
  },

  











  openNewBrowserWindow(options={}) {
    let argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";
    let features = "chrome,dialog=no,all";

    if (options.private) {
      features += ",private";
    }

    if (options.hasOwnProperty("remote")) {
      let remoteState = options.remote ? "remote" : "non-remote";
      features += `,${remoteState}`;
    }

    let win = Services.ww.openWindow(
      null, Services.prefs.getCharPref("browser.chromeURL"), "_blank",
      features, argString);

    
    
    
    return TestUtils.topicObserved("browser-delayed-startup-finished",
                                   subject => subject == win).then(() => win);
  },

  








  closeWindow(win) {
    return new Promise(resolve => {
      function observer(subject, topic, data) {
        if (topic == "domwindowclosed" && subject === win) {
          Services.ww.unregisterNotification(observer);
          resolve();
        }
      }
      Services.ww.registerNotification(observer);
      win.close();
    });
  },

  


























  waitForEvent(subject, eventName, checkFn) {
    return new Promise((resolve, reject) => {
      subject.addEventListener(eventName, function listener(event) {
        try {
          if (checkFn && !checkFn(event)) {
            return;
          }
          subject.removeEventListener(eventName, listener);
          resolve(event);
        } catch (ex) {
          try {
            subject.removeEventListener(eventName, listener);
          } catch (ex2) {
            
          }
          reject(ex);
        }
      });
    });
  },

  






















  synthesizeMouse(target, offsetX, offsetY, event, browser)
  {
    return new Promise(resolve => {
      let mm = browser.messageManager;
      mm.addMessageListener("Test:SynthesizeMouseDone", function mouseMsg(message) {
        mm.removeMessageListener("Test:SynthesizeMouseDone", mouseMsg);
        resolve(message.data.defaultPrevented);
      });

      let cpowObject = null;
      if (typeof target != "string") {
        cpowObject = target;
        target = null;
      }

      mm.sendAsyncMessage("Test:SynthesizeMouse",
                          {target, target, x: offsetX, y: offsetY, event: event},
                          {object: cpowObject});
    });
  },

  



  synthesizeMouseAtCenter(target, event, browser)
  {
    
    event.centered = true;
    return BrowserTestUtils.synthesizeMouse(target, 0, 0, event, browser);
  },

  




  synthesizeMouseAtPoint(offsetX, offsetY, event, browser)
  {
    return BrowserTestUtils.synthesizeMouse(null, offsetX, offsetY, event, browser);
  }
};
