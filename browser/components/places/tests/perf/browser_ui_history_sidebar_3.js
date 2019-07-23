











































waitForExplicitFinish();

const TEST_IDENTIFIER = "ui-perf-test";
const TEST_SUITE = "places";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
         getService(Ci.nsIWindowMediator);
var win = wm.getMostRecentWindow("navigator:browser");
var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

var sidebar = document.getElementById("sidebar");

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
  name: "history_sidebar_bylastvisited",
  times: [],
  run: function() {
    var self = this;
    var start = Date.now();
    sidebar.addEventListener("load", function() {
      sidebar.removeEventListener("load", arguments.callee, true);
      executeSoon(function() {
        var duration = Date.now() - start;
        sidebar.contentDocument.getElementById("bylastvisited").doCommand();
        toggleSidebar("viewHistorySidebar", false);
        self.times.push(duration);
        if (self.times.length == TEST_REPEAT_COUNT)
          self.finish();
        else
          self.run();
      });
    }, true);
    toggleSidebar("viewHistorySidebar", true);
  },
  finish: function() {
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
  else
    finish();
}
