










"use strict";

this.EXPORTED_SYMBOLS = [
  "BrowserTestUtils",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://testing-common/TestUtils.jsm");

Cc["@mozilla.org/globalmessagemanager;1"]
  .getService(Ci.nsIMessageListenerManager)
  .loadFrameScript(
    "chrome://mochikit/content/tests/BrowserTestUtils/content-utils.js", true);







const DEFAULT_WAIT = 2000;


this.BrowserTestUtils = {
  








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

  








  openNewBrowserWindow(options) {
    let argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";
    let features = "chrome,dialog=no,all";

    if (options && options.private || false) {
      features += ",private";
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


  





















  waitForEvent(subject, eventName, timeoutMs, target) {
    return new Promise((resolve, reject) => {
      function listener(event) {
        if (target && target !== event.target) {
          return;
        }

        subject.removeEventListener(eventName, listener);
        clearTimeout(timerID);
        resolve(event);
      }

      timeoutMs = timeoutMs || DEFAULT_WAIT;
      let stack = new Error().stack;

      let timerID = setTimeout(() => {
        subject.removeEventListener(eventName, listener);
        reject(new Error(`${eventName} event timeout at ${stack}`));
      }, timeoutMs);


      subject.addEventListener(eventName, listener);
    });
  },
};
