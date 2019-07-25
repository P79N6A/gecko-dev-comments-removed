let testURL = chromeRoot + "browser_contentpopup.html";
messageManager.loadFrameScript(chromeRoot + "remote_contentpopup.js", true);

let newTab = null;


var gTests = [];
var gCurrentTest = null;

function test() {
  
  waitForExplicitFinish();

  
  messageManager.addMessageListener("pageshow", function(aMessage) {
    if (newTab && newTab.browser.currentURI.spec != "about:blank") {
      messageManager.removeMessageListener(aMessage.name, arguments.callee);
      BrowserUI.closeAutoComplete(true);
      setTimeout(runNextTest, 0);
    }
  });

  let startupInfo = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup).getStartupInfo();
  if (!("firstPaint" in startupInfo))
    waitFor(function() { newTab = Browser.addTab(testURL, true); }, function() {
      let startupInfo = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup).getStartupInfo();
      return ("firstPaint" in startupInfo);
    }, Date.now() + 3000);
  else
    newTab = Browser.addTab(testURL, true);
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    try {
      

      
      Browser.closeTab(newTab);
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Show/Hide the content popup helper",

  run: function() {
    let popup = document.getElementById("form-helper-suggestions-container");
    popup.addEventListener("contentpopupshown", function(aEvent) {
      aEvent.target.removeEventListener(aEvent.type, arguments.callee, false);
      ok(!popup.hidden, "Content popup should be visible");
      waitFor(gCurrentTest.hidePopup, function() {
        return FormHelperUI._open;
      });
    }, false);

    AsyncTests.waitFor("TestRemoteAutocomplete:Click",
                        { id: "input-datalist-1" }, function(json) {});
  },

  hidePopup: function() {
    let popup = document.getElementById("form-helper-suggestions-container");
    popup.addEventListener("contentpopuphidden", function(aEvent) {
      popup.removeEventListener("contentpopuphidden", arguments.callee, false);
      ok(popup.hidden, "Content popup should be hidden");
      waitFor(gCurrentTest.finish, function() {
        return !FormHelperUI._open;
      });
    }, false);

    
    FormHelperUI.hide();
  },

  finish: function() {
    runNextTest();
  }
});

