










































waitForExplicitFinish();

const TEST_IDENTIFIER = "ui-perf-test";
const TEST_SUITE = "places";

var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

var maxResults = Services.prefs.getIntPref("browser.urlbar.maxRichResults");
var onSearchComplete = gURLBar.onSearchComplete;

function add_visit(aURI, aDate) {
  var visitId = hs.addVisit(aURI,
                            aDate,
                            null, 
                            hs.TRANSITION_TYPED, 
                            false, 
                            0);
  return visitId;
}

function add_bookmark(aURI) {
  var bId = bs.insertBookmark(bs.unfiledBookmarksFolder, aURI,
                              bs.DEFAULT_INDEX, "bookmark/" + aURI.spec);
  return bId;
}

function make_test_report(testName, result, units) {
  return [TEST_IDENTIFIER, TEST_SUITE, testName, result, units||"ms"].join(":");
}


var ptests = [];



const TEST_REPEAT_COUNT = 6;


ptests.push({
  name: "open_locationbar_default",
  times: [],
  run: function() {
    var self = this;
    var start = Date.now();
    var acItemsCount = 1;
    gURLBar.onSearchComplete = function() {
      executeSoon(function() {
        var duration = Date.now() - start;
        self.times.push(duration);
        if (self.times.length == TEST_REPEAT_COUNT)
          self.finish();
        else
          self.run();
      });
    };
    window.focus();
    gURLBar.focus();
    var synthesizeSearch = function() {
      EventUtils.synthesizeKey("VK_BACK_SPACE", {});
      EventUtils.synthesizeKey("e", {});
    };
    waitForFocus(synthesizeSearch);
  },
  finish: function() {
    gURLBar.value = "";
    processTestResult(this);
    setTimeout(runNextTest, 0);
  }
});

function processTestResult(aTest) {
  aTest.times.sort();  
  aTest.times.pop();   
  aTest.times.shift(); 
  var totalDuration = aTest.times.reduce(function(time, total){ return time + total; });
  var avgDuration = totalDuration/aTest.times.length;
  var report = make_test_report(aTest.name, avgDuration);
  ok(true, report);
}

function test() {
  
  setTimeout(runNextTest, 0);
}

function runNextTest() {
  if (ptests.length > 0)
    ptests.shift().run();
  else {
    gURLBar.onSearchComplete = onSearchComplete;
    finish();
  }
}
