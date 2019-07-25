



var testURL_01 = "http://mochi.test:8888/browser/mobile/chrome/browser_blank_01.html";


var gTests = [];
var gCurrentTest = null;



function test() {
  
  
  waitForExplicitFinish();

  
  runNextTest();
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    try {
    }
    finally {
      
      finish();
    }
  }
}




function waitForObserve(name, callback) {
  var observerService = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);
  var observer = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
    observe: function(subject, topic, data) {
      observerService.removeObserver(observer, name);
      observer = null;
      callback(subject, topic, data);
    }
  };

  observerService.addObserver(observer, name, false);
}



gTests.push({
  desc: "Test history being added with page visit",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_01, true);
    waitForObserve("uri-visit-saved", function(subject, topic, data) {
      let uri = subject.QueryInterface(Ci.nsIURI);
      ok(uri.spec == testURL_01, "URI was saved to history");
      Browser.closeTab(gCurrentTest._currentTab);
      runNextTest();
    });
  }
});
