Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BrowserTestUtils",
  "resource://testing-common/BrowserTestUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ContentTask",
  "resource://testing-common/ContentTask.jsm");

const REFERRER_URL_BASE = "/browser/browser/base/content/test/referrer/";
const REFERRER_POLICYSERVER_URL =
  "test1.example.com" + REFERRER_URL_BASE + "file_referrer_policyserver.sjs";

let gTestWindow = null;









let _referrerTests = [
  
  
  {
    fromScheme: "http://",
    toScheme: "http://",
    result: "http://test1.example.com/browser"  
  },
  {
    fromScheme: "https://",
    toScheme: "http://",
    result: ""  
  },
  
  
  {
    fromScheme: "https://",
    toScheme: "http://",
    policy: "origin",
    result: "https://test1.example.com"  
  },
  {
    fromScheme: "https://",
    toScheme: "http://",
    policy: "origin",
    rel: "noreferrer",
    result: ""  
  },
  
  
  
  {
    fromScheme: "https://",
    toScheme: "https://",
    policy: "origin-when-cross-origin",
    result: "https://test1.example.com/browser"  
  },
  {
    fromScheme: "http://",
    toScheme: "https://",
    policy: "origin-when-cross-origin",
    result: "http://test1.example.com"  
  },
];






function getReferrerTest(aTestNumber) {
  return _referrerTests[aTestNumber];
}






function getReferrerTestDescription(aTestNumber) {
  let test = getReferrerTest(aTestNumber);
  return "policy=[" + test.policy + "] " +
         "rel=[" + test.rel + "] " +
         test.fromScheme + " -> " + test.toScheme;
}







function clickTheLink(aWindow, aLinkId, aOptions) {
  return BrowserTestUtils.synthesizeMouseAtCenter(
    "#" + aLinkId, aOptions, aWindow.gBrowser.selectedBrowser);
}







function referrerResultExtracted(aWindow) {
  return ContentTask.spawn(aWindow.gBrowser.selectedBrowser, {}, function() {
    return content.document.getElementById("testdiv").textContent;
  });
}







function delayedStartupFinished(aWindow) {
  return new Promise(function(resolve) {
    Services.obs.addObserver(function observer(aSubject, aTopic) {
      if (aWindow == aSubject) {
        Services.obs.removeObserver(observer, aTopic);
        resolve();
      }
    }, "browser-delayed-startup-finished", false);
  });
}







function someTabLoaded(aWindow) {
  return new Promise(function(resolve) {
    aWindow.gBrowser.addEventListener("load", function onLoad(aEvent) {
      let tab = aWindow.gBrowser._getTabForContentWindow(
          aEvent.target.defaultView.top);
      if (tab) {
        aWindow.gBrowser.removeEventListener("load", onLoad, true);
        resolve(tab);
      }
    }, true);
  });
}






function newWindowOpened() {
  return TestUtils.topicObserved("browser-delayed-startup-finished")
                  .then(([win]) => win);
}








function contextMenuOpened(aWindow, aLinkId) {
  return new Promise(function(resolve) {
    aWindow.document.addEventListener("popupshown",
                                      function handleMenu(aEvent) {
      aWindow.document.removeEventListener("popupshown", handleMenu, false);
      resolve(aEvent.target);
    }, false);

    
    clickTheLink(aWindow, aLinkId, {type: "contextmenu", button: 2});
  });
}







function doContextMenuCommand(aWindow, aMenu, aItemId) {
  let command = aWindow.document.getElementById(aItemId);
  command.doCommand();
  aMenu.hidePopup();
}







function referrerTestCaseLoaded(aTestNumber) {
  let test = getReferrerTest(aTestNumber);
  let url = test.fromScheme + REFERRER_POLICYSERVER_URL +
            "?scheme=" + escape(test.toScheme) +
            "&policy=" + escape(test.policy || "") +
            "&rel=" + escape(test.rel || "");
  var browser = gTestWindow.gBrowser;
  browser.selectedTab = browser.addTab(url);
  return BrowserTestUtils.browserLoaded(browser.selectedBrowser);
}








function checkReferrerAndStartNextTest(aTestNumber, aNewWindow, aNewTab,
                                       aStartTestCase) {
  referrerResultExtracted(aNewWindow || gTestWindow).then(function(result) {
    
    let test = getReferrerTest(aTestNumber);
    let desc = getReferrerTestDescription(aTestNumber);
    is(result, test.result, desc);

    
    aNewTab && (aNewWindow || gTestWindow).gBrowser.removeTab(aNewTab);
    aNewWindow && aNewWindow.close();
    is(gTestWindow.gBrowser.tabs.length, 2, "two tabs open");
    gTestWindow.gBrowser.removeTab(gTestWindow.gBrowser.tabs[1]);

    
    var nextTestNumber = aTestNumber + 1;
    if (getReferrerTest(nextTestNumber)) {
      referrerTestCaseLoaded(nextTestNumber).then(function() {
        aStartTestCase(nextTestNumber);
      });
    } else {
      finish();
    }
  });
}







function startReferrerTest(aStartTestCase) {
  waitForExplicitFinish();

  
  gTestWindow = openDialog(location, "", "chrome,all,dialog=no", "about:blank");
  registerCleanupFunction(function() {
    gTestWindow && gTestWindow.close();
  });

  
  delayedStartupFinished(gTestWindow).then(function() {
    referrerTestCaseLoaded(0).then(function() {
      aStartTestCase(0);
    });
  });
}
