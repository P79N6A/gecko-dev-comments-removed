







var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
function uri(spec) {
  return ios.newURI(spec, null, null);
}

var sidebar = document.getElementById("sidebar");


var pages = [
  "http://sidebar.mozilla.org/a",
  "http://sidebar.mozilla.org/b",
  "http://sidebar.mozilla.org/c",
  "http://www.mozilla.org/d",
];

const FILTERED_COUNT = 1;

function test() {
  waitForExplicitFinish();

  
  PlacesTestUtils.clearHistory().then(continue_test);
}

function continue_test() {
  
  var time = Date.now();
  var pagesLength = pages.length;
  var places = [];
  for (var i = 0; i < pagesLength; i++) {
    places.push({uri: uri(pages[i]), visitDate: (time - i) * 1000,
                 transition: hs.TRANSITION_TYPED});
  }
  PlacesTestUtils.addVisits(places).then(() => {
    SidebarUI.show("viewHistorySidebar");
  });

  sidebar.addEventListener("load", function() {
    sidebar.removeEventListener("load", arguments.callee, true);
    executeSoon(function() {
      
      sidebar.contentDocument.getElementById("bylastvisited").doCommand();
      check_sidebar_tree_order(pages.length);
      var searchBox = sidebar.contentDocument.getElementById("search-box");
      ok(searchBox, "search box is in context");
      searchBox.value = "sidebar.mozilla";
      searchBox.doCommand();
      check_sidebar_tree_order(pages.length - FILTERED_COUNT);
      searchBox.value = "";
      searchBox.doCommand();
      check_sidebar_tree_order(pages.length);

      
      SidebarUI.hide();
      PlacesTestUtils.clearHistory().then(finish);
    });
  }, true);
}

function check_sidebar_tree_order(aExpectedRows) {
  var tree = sidebar.contentDocument.getElementById("historyTree");
  var treeView = tree.view;
  var rc = treeView.rowCount;
  var columns = tree.columns;
  is(columns.count, 1, "There should be only 1 column in the sidebar");
  var found = 0;
  for (var r = 0; r < rc; r++) {
    var node = treeView.nodeForTreeIndex(r);
    
    
    if (pages.indexOf(node.uri) == -1)
      continue;
    is(node.uri, pages[r], "Node is in correct position based on its visit date");
    found++;
  }
  ok(found, aExpectedRows, "Found all expected results");
}
