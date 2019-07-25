let testURL_01 = chromeRoot + "browser_formsZoom.html";

let baseURI = "http://mochi.test:8888/browser/mobile/chrome/";
let testURL_02 = baseURI + "browser_formsZoom.html";
messageManager.loadFrameScript(baseURI + "remote_formsZoom.js", true);


var gTests = [];
var gCurrentTest = null;



function test() {
  
  
  waitForExplicitFinish();
  
  runNextTest();
}

function waitForPageShow(aPageURL, aCallback) {
  messageManager.addMessageListener("pageshow", function(aMessage) {
    if (aMessage.target.currentURI.spec == aPageURL) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      setTimeout(function() { aCallback(); }, 0);
    }
  });
};



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    setTimeout(gCurrentTest.run, 0);
  }
  else {
    
    try {
      
    }
    finally {
      
      finish();
    }
  }
}

function waitForZoom(aCallback) {
  if (AnimatedZoom.isZooming()) {
    let self = this;
    window.addEventListener("AnimatedZoomEnd", function() {
      window.removeEventListener("AnimatedZoomEnd", arguments.callee, false);
      setTimeout(aCallback, 0);
    }, false);
  }
  else setTimeout(aCallback, 0);
}

function isElementVisible(aElement) {
  let elementRect = Rect.fromRect(aElement.rect);
  let caretRect = Rect.fromRect(aElement.caretRect);

  let browser = getBrowser();
  let zoomRect = Rect.fromRect(browser.getBoundingClientRect());
  let scroll = browser.getPosition();
  let browserRect = new Rect(scroll.x, scroll.y, zoomRect.width, zoomRect.height);

  info("CanZoom: " +Browser.selectedTab.allowZoom);

  info("Browser rect: " + browserRect + " - scale: " + browser.scale);
  info("Element rect: " + elementRect + " - caret rect: " + caretRect);
  info("Scale element rect: " + elementRect.clone().scale(browser.scale, browser.scale) + " - scale caretRect: " + caretRect.clone().scale(browser.scale, browser.scale));
  info("Resulting zoom rect: " + Browser._getZoomRectForPoint(elementRect.center().x, elementRect.y, browser.scale));

  let isCaretSyncEnabled = Services.prefs.getBoolPref("formhelper.autozoom.caret");
  if (isCaretSyncEnabled) {
    ok(browserRect.contains(caretRect.clone().scale(browser.scale, browser.scale)), "Caret rect should be completely visible");
  }
  else {
    elementRect = elementRect.clone().scale(browser.scale, browser.scale);
    let resultRect = browserRect.intersect(elementRect);
    ok(!resultRect.isEmpty() && elementRect.x > browserRect.x && elementRect.y > browserRect.y, "Element should be partially visible");
  }
}




gTests.push({
  desc: "Loading a page and Zoom into textarea field with caret sync enabled",
  elements: ["textarea-1", "textarea-2", "textarea-3", "textarea-4"],
  _currentTab: null,

  run: function() {
    Services.prefs.setBoolPref("formhelper.autozoom.caret", false);
    gCurrentTest._currentTab = BrowserUI.newTab(testURL_01);

    waitForPageShow(testURL_01, function() { gCurrentTest.zoomNext(); });
  },

  zoomNext: function() {
    let id = this.elements.shift();
    if (!id) {
      BrowserUI.closeTab();
      runNextTest();
      return;
    }

    info("Zooming to " + id);
    AsyncTests.waitFor("FormAssist:Show", { id: id }, function(json) {
      waitForZoom(function() {
        isElementVisible(json.current);
        gCurrentTest.zoomNext();
      });
    });
  }
});



gTests.push({
  desc: "Loading a page and Zoom into textarea field with caret sync enabled",
  elements: ["textarea-1", "textarea-2", "textarea-3", "textarea-4"],
  _currentTab: null,

  run: function() {
    Services.prefs.setBoolPref("formhelper.autozoom.caret", true);
    gCurrentTest._currentTab = BrowserUI.newTab(testURL_01);

    waitForPageShow(testURL_01, function() { gCurrentTest.zoomNext(); });
  },

  zoomNext: function() {
    let id = this.elements.shift();
    if (!id) {
      BrowserUI.closeTab();
      runNextTest();
      return;
    }

    AsyncTests.waitFor("FormAssist:Show", { id: id }, function(json) {
      waitForZoom(function() {
        isElementVisible(json.current);
        gCurrentTest.zoomNext();
      });
    });
  }
});



gTests.push({
  desc: "Loading a remote page and Zoom into textarea field with caret sync disabled",
  elements: ["textarea-1", "textarea-2", "textarea-3", "textarea-4"],
  _currentTab: null,

  run: function() {
    Services.prefs.setBoolPref("formhelper.autozoom.caret", false);
    gCurrentTest._currentTab = BrowserUI.newTab(testURL_02);

    waitForPageShow(testURL_02, function() { gCurrentTest.zoomNext(); });
  },

  zoomNext: function() {
    let id = this.elements.shift();
    if (!id) {
      BrowserUI.closeTab();
      runNextTest();
      return;
    }

    AsyncTests.waitFor("FormAssist:Show", { id: id }, function(json) {
      waitForZoom(function() {
        isElementVisible(json.current);
        gCurrentTest.zoomNext();
      });
    });
  }
});



gTests.push({
  desc: "Loading a remote page and Zoom into textarea field with caret sync enabled",
  elements: ["textarea-1", "textarea-2"],
  _currentTab: null,

  run: function() {
    Services.prefs.setBoolPref("formhelper.autozoom.caret", true);
    gCurrentTest._currentTab = BrowserUI.newTab(testURL_02);

    waitForPageShow(testURL_02, function() { gCurrentTest.zoomNext(); });
  },

  zoomNext: function() {
    let id = this.elements.shift();
    if (!id) {
      todo(false, "textarea-3 caret should be synced, but for some reason it is not");
      todo(false, "textarea-4 caret should be synced, but for some reason it is not");
      BrowserUI.closeTab();
      runNextTest();
      return;
    }

    AsyncTests.waitFor("FormAssist:Show", { id: id }, function(json) {
      waitForZoom(function() {
        isElementVisible(json.current);
        gCurrentTest.zoomNext();
      });
    });
  }
});

