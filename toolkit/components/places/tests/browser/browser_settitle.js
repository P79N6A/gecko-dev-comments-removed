




gBrowser.selectedTab = gBrowser.addTab();

function finishAndCleanUp()
{
  gBrowser.removeCurrentTab();
  PlacesTestUtils.clearHistory().then(finish);
}




function load(href, callback)
{
  content.location.href = href;
  gBrowser.addEventListener("load", function() {
    if (content.location.href == href) {
      gBrowser.removeEventListener("load", arguments.callee, true);
      if (callback)
        callback();
    }
  }, true);
}

var conn = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;




function getColumn(table, column, fromColumnName, fromColumnValue)
{
  var stmt = conn.createStatement(
    `SELECT ${column} FROM ${table} WHERE ${fromColumnName} = :val
     LIMIT 1`);
  try {
    stmt.params.val = fromColumnValue;
    stmt.executeStep();
    return stmt.row[column];
  }
  finally {
    stmt.finalize();
  }
}

function test()
{
  
  

  waitForExplicitFinish();

  
  var historyObserver = {
    data: [],
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {},
    onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID,
                      aTransitionType) {
    },
    onTitleChanged: function(aURI, aPageTitle, aGUID) {
      this.data.push({ uri: aURI, title: aPageTitle, guid: aGUID });

      
      
      
      
      

      PlacesUtils.history.removeObserver(this);
      confirmResults(this.data);
    },
    onDeleteURI: function() {},
    onClearHistory: function() {},
    onPageChanged: function() {},
    onDeleteVisits: function() {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };
  PlacesUtils.history.addObserver(historyObserver, false);

  load("http://example.com/tests/toolkit/components/places/tests/browser/title1.html", function() {
    load("http://example.com/tests/toolkit/components/places/tests/browser/title2.html");
  });

  function confirmResults(data) {
    is(data[0].uri.spec, "http://example.com/tests/toolkit/components/places/tests/browser/title2.html");
    is(data[0].title, "Some title");
    is(data[0].guid, getColumn("moz_places", "guid", "url", data[0].uri.spec));

    data.forEach(function(item) {
      var title = getColumn("moz_places", "title", "url", data[0].uri.spec);
      is(title, item.title);
    });

    finishAndCleanUp();
  }
}
