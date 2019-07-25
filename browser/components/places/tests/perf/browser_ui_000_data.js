
















































waitForExplicitFinish();

const TEST_IDENTIFIER = "ui-perf-test";
const TEST_SUITE = "places";

var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

function add_visit(aURI, aDate) {
  var placeID = hs.addVisit(aURI,
                            aDate,
                            null, 
                            hs.TRANSITION_TYPED, 
                            false, 
                            0);
  return placeID;
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




ptests.push({
  run: function() {
    bs.runInBatchMode({
      runBatched: function(aUserData) {
        
        var days = 90;

        
        var total_visits = 300;
        var visits_per_day = total_visits/days;

        var visit_date_microsec = Date.now() * 1000;
        var day_counter = 0;
        
        var start = Date.now();
        for (var i = 0; i < days; i++) {
          visit_date_microsec -= 86400 * 1000 * 1000; 
          var spec = "http://example.com/" + visit_date_microsec;
          for (var j = 0; j < visits_per_day; j++) {
            var uri = Services.io.newURI(spec + j, null, null);
            add_visit(uri, visit_date_microsec);
          }
        }
        var duration = Date.now() - start;
        var report = make_test_report("add_visits", duration);
        ok(true, report);

        
        var bookmarks_total = total_visits/10; 
        var bookmarks_per_day = bookmarks_total/days;

        
        visit_date_microsec = Date.now() * 1000;
        var bookmark_counter = 0;
        start = Date.now();
        for (var i = 0; i < days; i++) {
          visit_date_microsec -= 86400 * 1000 * 1000; 
          var spec = "http://example.com/" + visit_date_microsec;
          for (var j = 0; j < visits_per_day; j++) {
            var uri = Services.io.newURI(spec + j, null, null);
            if (bookmark_counter < bookmarks_per_day) {
              add_bookmark(uri);
              bookmark_counter++;
            }
            else
              bookmark_counter = 0;
          }
        }
        duration = Date.now() - start;
        report = make_test_report("add_bookmarks", duration);
        ok(true, report);
        runNextTest();
      }
    }, null);
  }
});

function test() {
  
  runNextTest();
}

function runNextTest() {
  if (ptests.length > 0)
    ptests.shift().run();
  else
    finish();
}
