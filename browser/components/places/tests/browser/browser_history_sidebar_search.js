








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var ios = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
function uri(spec) {
  return ios.newURI(spec, null, null);
}

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


var pages = [
  "http://sidebar.mozilla.org/a",
  "http://sidebar.mozilla.org/b",
  "http://sidebar.mozilla.org/c",
  "http://www.mozilla.org/d",
];

const FILTERED_COUNT = 1;

function test() {
  waitForExplicitFinish();

  
  bh.removeAllPages();

  
  var time = Date.now();
  for (var i = 0; i < pages.length; i++) {
    add_visit(uri(pages[i]), (time - i) * 1000);
  }

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

      
      toggleSidebar("viewHistorySidebar", false);
      bh.removeAllPages();

      finish();
    });
  }, true);
  toggleSidebar("viewHistorySidebar", true);
}

function check_sidebar_tree_order(aExpectedRows) {
  var tree = sidebar.contentDocument.getElementById("historyTree");
  var treeView = tree.view;
  var rc = treeView.rowCount;
  is(rc, aExpectedRows, "All expected tree rows are present");
  var columns = tree.columns;
  is(columns.count, 1, "There should be only 1 column in the sidebar");
  for (var r = 0; r < rc; r++) {
    var node = treeView.nodeForTreeIndex(r);
    is(node.uri, pages[r], "Node is in correct position based on its visit date");
  }
}
