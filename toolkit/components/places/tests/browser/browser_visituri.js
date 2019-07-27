




gBrowser.selectedTab = gBrowser.addTab();

function finishAndCleanUp()
{
  gBrowser.removeCurrentTab();
  promiseClearHistory().then(finish);
}




function waitForObserve(name, callback)
{
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




function waitForLoad(callback)
{
  gBrowser.addEventListener("DOMContentLoaded", function() {
    gBrowser.removeEventListener("DOMContentLoaded", arguments.callee, true);
    callback();
  }, true);
}

var conn = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;




function getColumn(table, column, fromColumnName, fromColumnValue)
{
  let sql = `SELECT ${column}
             FROM ${table}
             WHERE ${fromColumnName} = :val
             LIMIT 1`;
  let stmt = conn.createStatement(sql);
  try {
    stmt.params.val = fromColumnValue;
    ok(stmt.executeStep(), "Expect to get a row");
    return stmt.row[column];
  }
  finally {
    stmt.reset();
  }
}

function test()
{
  
  

  waitForExplicitFinish();

  
  
  var expectedUrls = [
    "http://example.com/tests/toolkit/components/places/tests/browser/begin.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/redirect_twice.sjs",
    "http://example.com/tests/toolkit/components/places/tests/browser/redirect_once.sjs",
    "http://example.com/tests/toolkit/components/places/tests/browser/final.html"
  ];
  var currentIndex = 0;

  waitForObserve("uri-visit-saved", function(subject, topic, data) {
    var uri = subject.QueryInterface(Ci.nsIURI);
    var expected = expectedUrls[currentIndex];
    is(uri.spec, expected, "Saved URL visit " + uri.spec);

    var placeId = getColumn("moz_places", "id", "url", uri.spec);
    var fromVisitId = getColumn("moz_historyvisits", "from_visit", "place_id", placeId);

    if (currentIndex == 0) {
      is(fromVisitId, 0, "First visit has no from visit");
    }
    else {
      var lastVisitId = getColumn("moz_historyvisits", "place_id", "id", fromVisitId);
      var fromVisitUrl = getColumn("moz_places", "url", "id", lastVisitId);
      is(fromVisitUrl, expectedUrls[currentIndex - 1],
         "From visit was " + expectedUrls[currentIndex - 1]);
    }

    currentIndex++;
    if (currentIndex < expectedUrls.length) {
      waitForObserve("uri-visit-saved", arguments.callee);
    }
    else {
      finishAndCleanUp();
    }
  });

  
  content.location.href = "http://example.com/tests/toolkit/components/places/tests/browser/begin.html";
  waitForLoad(function() {
    EventUtils.sendMouseEvent({type: 'click'}, 'clickme', content.window);
    waitForLoad(function() {
      content.location.href = "about:blank";
    });
  });
}
