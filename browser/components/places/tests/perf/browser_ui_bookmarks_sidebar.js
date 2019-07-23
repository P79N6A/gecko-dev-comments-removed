











































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
  name: "open_bookmarks_sidebar",
  times: [],
  run: function() {
    var self = this;
    var start = Date.now();
    var sb = document.getElementById("sidebar");
    sb.addEventListener("load", function() {
      var duration = Date.now() - start;
      sb.removeEventListener("load", arguments.callee, true);
      toggleSidebar("viewBookmarksSidebar", false);
      self.times.push(duration);
      if (self.times.length == TEST_REPEAT_COUNT)
        self.finish();
      else
        self.run();
    }, true);
    toggleSidebar("viewBookmarksSidebar", true);
  },
  finish: function() {
    this.times.sort();  
    this.times.pop();   
    this.times.shift(); 
    var totalDuration = this.times.reduce(function(time, total){ return time + total; });
    var avgDuration = totalDuration/this.times.length;
    var report = make_test_report("open_bookmarks_sidebar", avgDuration);
    ok(true, report);
    setTimeout(runNextTest, 0);
  }
});

function test() {
  
  setTimeout(runNextTest, 0);
}

function runNextTest() {
  if (ptests.length > 0) {
    ptests.shift().run();
  }
  else
    finish();
}
