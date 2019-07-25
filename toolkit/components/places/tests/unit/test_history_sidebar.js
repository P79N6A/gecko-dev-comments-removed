







































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);












function add_normalized_visit(aURI, aTime, aDayOffset) {
  var dateObj = new Date(aTime);
  
  dateObj.setHours(0);
  dateObj.setMinutes(0);
  dateObj.setSeconds(0);
  dateObj.setMilliseconds(0);
  
  var previousDateObj = new Date(dateObj.getTime() + aDayOffset * 86400000);
  var DSTCorrection = (dateObj.getTimezoneOffset() -
                       previousDateObj.getTimezoneOffset()) * 60 * 1000;
  
  var PRTimeWithOffset = (previousDateObj.getTime() - DSTCorrection) * 1000;
  var timeInMs = new Date(PRTimeWithOffset/1000);
  print("Adding visit to " + aURI.spec + " at " + timeInMs);
  var visitId = hs.addVisit(aURI,
                            PRTimeWithOffset,
                            null,
                            hs.TRANSITION_TYPED, 
                            false, 
                            0);
  do_check_true(visitId > 0);
  return visitId;
}

function days_for_x_months_ago(aNowObj, aMonths) {
  var oldTime = new Date();
  
  
  oldTime.setDate(1);
  oldTime.setMonth(aNowObj.getMonth() - aMonths);
  oldTime.setHours(0);
  oldTime.setMinutes(0);
  oldTime.setSeconds(0);
  
  return parseInt((aNowObj - oldTime) / (1000*60*60*24)) + 2;
}

var nowObj = new Date();


var containers = [
  { label: "Today"               , offset: 0                                 , visible: true },
  { label: "Yesterday"           , offset: -1                                , visible: true },
  { label: "Last 7 days"         , offset: -3                                , visible: true },
  { label: "This month"          , offset: -8                                , visible: nowObj.getDate() > 8 },
  { label: ""                    , offset: -days_for_x_months_ago(nowObj, 0) , visible: true },
  { label: ""                    , offset: -days_for_x_months_ago(nowObj, 1) , visible: true },
  { label: ""                    , offset: -days_for_x_months_ago(nowObj, 2) , visible: true },
  { label: ""                    , offset: -days_for_x_months_ago(nowObj, 3) , visible: true },
  { label: ""                    , offset: -days_for_x_months_ago(nowObj, 4) , visible: true },
  { label: "Older than 6 months" , offset: -days_for_x_months_ago(nowObj, 5) , visible: true },
];

var visibleContainers = containers.filter(
  function(aContainer) {return aContainer.visible});




function fill_history() {
  print("\n\n*** TEST Fill History\n");
  
  
  for (var i = 0; i < containers.length; i++) {
    var container = containers[i];
    var testURI = uri("http://mirror"+i+".mozilla.com/b");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".mozilla.com/a");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".google.com/b");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    var testURI = uri("http://mirror"+i+".google.com/a");
    add_normalized_visit(testURI, nowObj.getTime(), container.offset);
    
    
    
    check_visit(container.offset);
  }

  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = hs.getNewQuery();

  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  var cc = root.childCount;
  print("Found containers:");
  var previousLabels = [];
  for (var i = 0; i < cc; i++) {
    var container = visibleContainers[i];
    var node = root.getChild(i);
    print(node.title);
    if (container.label)
      do_check_eq(node.title, container.label);
    
    do_check_eq(previousLabels.indexOf(node.title), -1);
    previousLabels.push(node.title);
  }
  do_check_eq(cc, visibleContainers.length);
  root.containerOpen = false;
}





function check_visit(aOffset) {
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;

  var unexpected = [];
  switch (aOffset) {
    case 0:
      unexpected = ["Yesterday", "Last 7 days", "This month"];
      break;
    case -1:
      unexpected = ["Last 7 days", "This month"];
      break;
    case -3:
      unexpected = ["This month"];
      break;
    default:
      
  }

  print("Found containers:");
  for (var i = 0; i < cc; i++) {
    var node = root.getChild(i);
    print(node.title);
    do_check_eq(unexpected.indexOf(node.title), -1);
  }

  root.containerOpen = false;
}





function test_RESULTS_AS_DATE_SITE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_DATE_SITE_QUERY\n");
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_SITE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 2);

  
  var site1 = dayNode.getChild(0)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site1.title, "mirror0.google.com");

  var site2 = dayNode.getChild(1)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site2.title, "mirror0.mozilla.com");

  site1.containerOpen = true;
  do_check_eq(site1.childCount, 2);

  
  var site1visit = site1.getChild(0);
  do_check_eq(site1visit.uri, "http://mirror0.google.com/a");

  
  result.sortingMode = options.SORT_BY_TITLE_DESCENDING;

  
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 2);

  
  var site1 = dayNode.getChild(0)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site1.title, "mirror0.google.com");

  var site2 = dayNode.getChild(1)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(site2.title, "mirror0.mozilla.com");

  site1.containerOpen = true;
  do_check_eq(site1.childCount, 2);

  
  var site1visit = site1.getChild(0);
  do_check_eq(site1visit.uri, "http://mirror0.google.com/b");

  site1.containerOpen = false;
  dayNode.containerOpen = false;
  root.containerOpen = false;
}




function test_RESULTS_AS_DATE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_DATE_QUERY\n");
  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_DATE_QUERY;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  var cc = root.childCount;
  do_check_eq(cc, visibleContainers.length);
  print("Found containers:");
  for (var i = 0; i < cc; i++) {
    var container = visibleContainers[i];
    var node = root.getChild(i);
    print(node.title);
    if (container.label)
      do_check_eq(node.title, container.label);
  }

  
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 4);

  
  var visit1 = dayNode.getChild(0);
  do_check_eq(visit1.uri, "http://mirror0.google.com/a");

  var visit2 = dayNode.getChild(3);
  do_check_eq(visit2.uri, "http://mirror0.mozilla.com/b");

  
  result.sortingMode = options.SORT_BY_TITLE_DESCENDING;

  
  var dayNode = root.getChild(0)
                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dayNode.containerOpen = true;
  do_check_eq(dayNode.childCount, 4);

  
  var visit1 = dayNode.getChild(0);
  do_check_eq(visit1.uri, "http://mirror0.mozilla.com/b");

  var visit2 = dayNode.getChild(3);
  do_check_eq(visit2.uri, "http://mirror0.google.com/a");

  dayNode.containerOpen = false;
  root.containerOpen = false;
}




function test_RESULTS_AS_SITE_QUERY() {
  print("\n\n*** TEST RESULTS_AS_SITE_QUERY\n");
  
  var itemId = bs.insertBookmark(bs.toolbarFolder, uri("http://foobar"),
                                 bs.DEFAULT_INDEX, "");

  var options = hs.getNewQueryOptions();
  options.resultType = options.RESULTS_AS_SITE_QUERY;
  options.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, containers.length * 2);

















  
  var siteNode = root.getChild(6)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(siteNode.title, "mirror3.google.com");

  siteNode.containerOpen = true;
  do_check_eq(siteNode.childCount, 2);

  
  var visitNode = siteNode.getChild(0);
  do_check_eq(visitNode.uri, "http://mirror3.google.com/a");

  
  result.sortingMode = options.SORT_BY_TITLE_DESCENDING;
  var siteNode = root.getChild(6)
                     .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(siteNode.title, "mirror3.google.com");

  siteNode.containerOpen = true;
  do_check_eq(siteNode.childCount, 2);

  
  var visit = siteNode.getChild(0);
  do_check_eq(visit.uri, "http://mirror3.google.com/b");

  siteNode.containerOpen = false;
  root.containerOpen = false;

  
  bs.removeItem(itemId);
}




function test_date_liveupdate(aResultType) {
  var midnight = nowObj;
  midnight.setHours(0);
  midnight.setMinutes(0);
  midnight.setSeconds(0);
  midnight.setMilliseconds(0);

  
  var options = hs.getNewQueryOptions();
  options.resultType = aResultType;
  var query = hs.getNewQuery();
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  do_check_eq(root.childCount, visibleContainers.length);
  
  hs.removePagesByTimeframe(midnight.getTime() * 1000, Date.now() * 1000);
  do_check_eq(root.childCount, visibleContainers.length - 1);

  
  
  var last7Days = root.getChild(1)
                      .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  last7Days.containerOpen = true;

  
  
  add_normalized_visit(uri("http://www.mozilla.org/"), nowObj.getTime(), 0);
  do_check_eq(root.childCount, visibleContainers.length);

  last7Days.containerOpen = false;
  root.containerOpen = false;

  
  var itemId = bs.insertBookmark(bs.toolbarFolder,
                                 uri("place:type=" + aResultType),
                                 bs.DEFAULT_INDEX, "");

  
  options = hs.getNewQueryOptions();
  query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  result = hs.executeQuery(query, options);
  root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 1);
  var dateContainer = root.getChild(0).QueryInterface(Ci.nsINavHistoryContainerResultNode);
  dateContainer.containerOpen = true;

  do_check_eq(dateContainer.childCount, visibleContainers.length);
  
  hs.removePagesByTimeframe(midnight.getTime() * 1000, Date.now() * 1000);
  do_check_eq(dateContainer.childCount, visibleContainers.length - 1);
  
  add_normalized_visit(uri("http://www.mozilla.org/"), nowObj.getTime(), 0);
  do_check_eq(dateContainer.childCount, visibleContainers.length);

  dateContainer.containerOpen = false;
  root.containerOpen = false;

  
  bs.removeItem(itemId);
}

function run_test() {
  
  if (nowObj.getHours() == 23 && nowObj.getMinutes() >= 50) {
    return;
  }

  fill_history();
  test_RESULTS_AS_DATE_SITE_QUERY();
  test_RESULTS_AS_DATE_QUERY();
  test_RESULTS_AS_SITE_QUERY();

  test_date_liveupdate(Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY);
  test_date_liveupdate(Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY);

  
  
  
  
  
}
