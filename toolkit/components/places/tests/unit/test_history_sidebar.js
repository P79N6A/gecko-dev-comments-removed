






































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 










function add_visit(aURI, aDayOffset) {
  var placeID = histsvc.addVisit(aURI,
                                 (Date.now() + aDayOffset*86400000) * 1000,
                                 null,
                                 histsvc.TRANSITION_TYPED, 
                                 false, 
                                 0);
  do_check_true(placeID > 0);
  return placeID;
}


var dayLabels = 
[ 
  "Today", 
  "Yesterday", 
  "2 days ago", 
  "3 days ago",
  "4 days ago",
  "5 days ago",
  "6 days ago",
  "Older than 6 days"
];


function fill_history() {
  const checkOlderOffset = 4;

  
  for (var i=checkOlderOffset; i<dayLabels.length; i++)
  {
    var testURI = uri("http://mirror"+i+".mozilla.com/b");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".mozilla.com/a");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".google.com/b");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".google.com/a");
    add_visit(testURI, -i);
  }

  var options = histsvc.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, dayLabels.length - checkOlderOffset);

  for (var i=checkOlderOffset; i<dayLabels.length; i++)
  {
    var node = root.getChild(i-checkOlderOffset);
    do_check_eq(node.title, dayLabels[i]);
  }


  
  
  root.containerOpen = false;
  
  
  for (var i=0; i<checkOlderOffset; i++)
  {
    var testURI = uri("http://mirror"+i+".mozilla.com/d");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".mozilla.com/c");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".google.com/d");
    add_visit(testURI, -i);
    var testURI = uri("http://mirror"+i+".google.com/c");
    add_visit(testURI, -i);
  }

  root.containerOpen = false;
}

function test_RESULTS_AS_DATE_SITE_QUERY() {

  var options = histsvc.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, dayLabels.length);

  
  for (var i=0; i<dayLabels.length; i++)
  {
    var node = root.getChild(i);
    do_check_eq(node.title, dayLabels[i]);
  }

  
  var dayNode = root.getChild(4).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 2);

  
  var site1 = dayNode.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site1.title, "mirror4.google.com");

  var site2 = dayNode.getChild(1).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site2.title, "mirror4.mozilla.com");

  site1.containerOpen = true;
  do_check_eq(site1.childCount, 2);

  
  var site1visit = site1.getChild(0);
  do_check_eq(site1visit.uri, "http://mirror4.google.com/a");

  site1.containerOpen = false;
  dayNode.containerOpen = false;
  root.containerOpen = false;

}

function test_RESULTS_AS_DATE_QUERY() {

  var options = histsvc.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_QUERY;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, dayLabels.length);

  
  for (var i=0; i<dayLabels.length; i++)
  {
    var node = root.getChild(i);
    do_check_eq(node.title, dayLabels[i]);
  }

  
  var dayNode = root.getChild(3).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 4);
  do_check_eq(dayNode.title, "3 days ago");

  
  var visit1 = dayNode.getChild(0);
  do_check_eq(visit1.uri, "http://mirror3.google.com/c");

  var visit2 = dayNode.getChild(3);
  do_check_eq(visit2.uri, "http://mirror3.mozilla.com/d");

  dayNode.containerOpen = false;
  root.containerOpen = false;
}

function test_RESULTS_AS_SITE_QUERY() {

  
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
  bmsvc.insertBookmark(bmsvc.toolbarFolder, uri("http://foobar"),
                       bmsvc.DEFAULT_INDEX, "");

  var options = histsvc.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_SITE_QUERY;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, dayLabels.length*2);

  
  var expectedResult = 
  [
    "mirror0.google.com",
    "mirror0.mozilla.com",
    "mirror1.google.com",
    "mirror1.mozilla.com",
    "mirror2.google.com",
    "mirror2.mozilla.com",
    "mirror3.google.com",
    "mirror3.mozilla.com",
    "mirror4.google.com",
    "mirror4.mozilla.com",
    "mirror5.google.com",   
    "mirror5.mozilla.com",
    "mirror6.google.com",
    "mirror6.mozilla.com",
    "mirror7.google.com",
    "mirror7.mozilla.com"
  ];

  
  var siteNode = root.getChild(dayLabels.length+2).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(siteNode.title,  expectedResult[dayLabels.length+2] );

  siteNode.containerOpen = true;
  do_check_eq(siteNode.childCount, 2);

  
  var visit = siteNode.getChild(0);
  do_check_eq(visit.uri, "http://mirror5.google.com/a");

  siteNode.containerOpen = false;
  root.containerOpen = false;
}


function run_test() {

  fill_history();
  test_RESULTS_AS_DATE_SITE_QUERY();
  test_RESULTS_AS_DATE_QUERY();
  test_RESULTS_AS_SITE_QUERY();

  
  
  
  
  
}
