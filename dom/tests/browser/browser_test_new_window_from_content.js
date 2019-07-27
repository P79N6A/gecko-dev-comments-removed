


"use strict";

































Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const kXULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const kContentDoc = "http://www.example.com/browser/dom/tests/browser/test_new_window_from_content_child.html";
const kContentScript = "chrome://mochitests/content/browser/dom/tests/browser/test_new_window_from_content_child.js";
const kNewWindowPrefKey = "browser.link.open_newwindow";
const kNewWindowRestrictionPrefKey = "browser.link.open_newwindow.restriction";
const kSameTab = "same tab";
const kNewWin = "new window";
const kNewTab = "new tab";

requestLongerTimeout(2);








const kWinOpenDefault = {



                  1: [kSameTab, kNewWin, kSameTab],
                  2: [kNewWin, kNewWin, kNewWin],
                  3: [kNewTab, kNewWin, kNewTab],
};

const kWinOpenNonDefault = {
  1: [kSameTab, kNewWin, kNewWin],
  2: [kNewWin, kNewWin, kNewWin],
  3: [kNewTab, kNewWin, kNewWin],
};

const kTargetBlank = {
  1: [kSameTab, kSameTab, kSameTab],
  2: [kNewWin, kNewWin, kNewWin],
  3: [kNewTab, kNewTab, kNewTab],
};



let originalNewWindowPref = Services.prefs.getIntPref(kNewWindowPrefKey);
let originalNewWindowRestrictionPref =
  Services.prefs.getIntPref(kNewWindowRestrictionPrefKey);

registerCleanupFunction(function() {
  Services.prefs.setIntPref(kNewWindowPrefKey, originalNewWindowPref);
  Services.prefs.setIntPref(kNewWindowRestrictionPrefKey,
                            originalNewWindowRestrictionPref);
  
  
  for (let tab of gBrowser.tabs) {
    let browser = gBrowser.getBrowserForTab(tab);
    if (browser.contentDocument.location == kContentDoc) {
      closeTab(tab);
    }
  }
});













function WindowOpenListener(aExpectedURI) {
  this._openDeferred = Promise.defer();
  this._closeDeferred = Promise.defer();
  this._expectedURI = aExpectedURI;
}

WindowOpenListener.prototype = {
  get openPromise() {
    return this._openDeferred.promise;
  },

  get closePromise() {
    return this._closeDeferred.promise;
  },

  onOpenWindow: function(aXULWindow) {
    let domWindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIDOMWindow);
    let location = domWindow.document.location;
    if (location == this._expectedURI) {
      let deferred = this._openDeferred;
      domWindow.addEventListener("load", function onWindowLoad() {
        domWindow.removeEventListener("load", onWindowLoad);
        deferred.resolve(domWindow);
      })
    }
  },

  onCloseWindow: function(aXULWindow) {
    this._closeDeferred.resolve();
  },
  onWindowTitleChange: function(aXULWindow, aNewTitle) {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowMediatorListener])
};







function loadAndSelectTestTab() {
  let tab = gBrowser.addTab(kContentDoc);
  gBrowser.selectedTab = tab;

  let browser = gBrowser.getBrowserForTab(tab);
  browser.messageManager.loadFrameScript(kContentScript, false);

  let deferred = Promise.defer();
  browser.addEventListener("DOMContentLoaded", function onBrowserLoad(aEvent) {
    browser.removeEventListener("DOMContentLoaded", onBrowserLoad);
    deferred.resolve(tab);
  });

  return deferred.promise;
}








function closeTab(aTab) {
  let deferred = Promise.defer();
  let browserMM = gBrowser.getBrowserForTab(aTab).messageManager;
  browserMM.sendAsyncMessage("TEST:allow-unload");
  browserMM.addMessageListener("TEST:allow-unload:done", function(aMessage) {
    gBrowser.removeTab(aTab);
    deferred.resolve();
  })
  return deferred.promise;
}







function clickInTab(aTab, aItemId) {
  let browserMM = gBrowser.getBrowserForTab(aTab).messageManager;
  browserMM.sendAsyncMessage("TEST:click-item", {
    details: aItemId,
  });
}

















function prepareForResult(aTab, aExpectation) {
  let deferred = Promise.defer();
  let browser = gBrowser.getBrowserForTab(aTab);

  switch(aExpectation) {
    case kSameTab:
      
      
      
      
      
      
      browser.addEventListener("DOMWillOpenModalDialog", function onModalDialog() {
        browser.removeEventListener("DOMWillOpenModalDialog", onModalDialog, true);
        executeSoon(() => {
          let stack = browser.parentNode;
          let dialogs = stack.getElementsByTagNameNS(kXULNS, "tabmodalprompt");
          dialogs[0].ui.button1.click()
          deferred.resolve();
        })
      }, true);
      break;
    case kNewWin:
      let listener = new WindowOpenListener("about:blank");
      Services.wm.addListener(listener);

      info("Waiting for a new about:blank window");
      listener.openPromise.then(function(aWindow) {
        info("Got the new about:blank window - closing it.");
        executeSoon(() => {
          aWindow.close();
        });
        listener.closePromise.then(() => {
          info("New about:blank window closed!");
          Services.wm.removeListener(listener);
          deferred.resolve();
        });
      });
      break;
    case kNewTab:
      gBrowser.tabContainer.addEventListener("TabOpen", function onTabOpen(aEvent) {
        let newTab = aEvent.target;
        let newBrowser = gBrowser.getBrowserForTab(newTab);
        if (newBrowser.contentDocument.location.href == "about:blank") {
          gBrowser.tabContainer.removeEventListener("TabOpen", onTabOpen);
          executeSoon(() => {
            gBrowser.removeTab(newTab);
            deferred.resolve();
          })
        }
      })
      break;
    default:
      ok(false, "prepareForResult can't handle an expectation of " + aExpectation)
      return;
  }

  return deferred.promise;
}











function testLinkWithMatrix(aLinkId, aMatrix) {
  return Task.spawn(function* () {
    let tab = yield loadAndSelectTestTab();
    
    
    
    
    
    
    

    for (let newWindowPref in aMatrix) {
      let expectations = aMatrix[newWindowPref];
      for (let i = 0; i < expectations.length; ++i) {
        let newWindowRestPref = i;
        let expectation = expectations[i];

        Services.prefs.setIntPref("browser.link.open_newwindow", newWindowPref);
        Services.prefs.setIntPref("browser.link.open_newwindow.restriction", newWindowRestPref);
        info("Clicking on " + aLinkId);
        info("Testing with browser.link.open_newwindow = " + newWindowPref + " and " +
             "browser.link.open_newwindow.restriction = " + newWindowRestPref);
        info("Expecting: " + expectation);
        let resultPromise = prepareForResult(tab, expectation);
        clickInTab(tab, aLinkId);
        yield resultPromise;
        ok(true, "Got expectation: " + expectation);
      }
    }
    yield closeTab(tab);
  });
}

add_task(function* test_window_open_with_defaults() {
  yield testLinkWithMatrix("winOpenDefault", kWinOpenDefault);
});

add_task(function* test_window_open_with_non_defaults() {
  yield testLinkWithMatrix("winOpenNonDefault", kWinOpenNonDefault);
});

add_task(function* test_target__blank() {
  yield testLinkWithMatrix("targetBlank", kTargetBlank);
});
