





































const DAY_MSEC = 86400000;
const MIN_MSEC = 60000;
const HOUR_MSEC = 3600000;

var beginTimeDate = new Date(2008, 0, 6, 8, 0, 0, 0);

var endTimeDate = new Date(2008, 0, 15, 21, 30, 0, 0);


var beginTime = beginTimeDate.getTime();
var endTime = endTimeDate.getTime();


var jan7_800 = (beginTime + DAY_MSEC) * 1000;
var jan6_815 = (beginTime + (MIN_MSEC * 15)) * 1000;
var jan11_800 = (beginTime + (DAY_MSEC * 5)) * 1000;
var jan14_2130 = (endTime - DAY_MSEC) * 1000;
var jan15_2045 = (endTime - (MIN_MSEC * 45)) * 1000;
var jan12_1730 = (endTime - (DAY_MSEC * 3) - (HOUR_MSEC*4)) * 1000;


var jan6_700 = (beginTime - HOUR_MSEC) * 1000;
var jan5_800 = (beginTime - DAY_MSEC) * 1000;
var dec27_800 = (beginTime - (DAY_MSEC * 10)) * 1000;
var jan15_2145 = (endTime + (MIN_MSEC * 15)) * 1000;
var jan16_2130 = (endTime + (DAY_MSEC)) * 1000;
var jan25_2130 = (endTime + (DAY_MSEC * 10)) * 1000;


beginTime *= 1000;
endTime *= 1000;




var goodAnnoName = "moz-test-places/testing123";
var val = "test";
var badAnnoName = "text/foo";




var testData = [

  
  {isInQuery: true, isVisit: true, isDetails: true, isPageAnnotation: true,
   uri: "http://foo.com/", annoName: goodAnnoName, annoVal: val,
   lastVisit: jan14_2130, title: "moz"},

  
  {isInQuery: true, isVisit: true, isDetails: true, title: "moz mozilla",
   uri: "http://foo.com/begin.html", lastVisit: beginTime},

  
  {isInQuery: true, isVisit: true, isDetails: true, title: "moz mozilla",
   uri: "http://foo.com/end.html", lastVisit: endTime},

  
  {isInQuery: true, isVisit: true, isDetails: true, title: "moz",
   isRedirect: true, uri: "http://foo.com/redirect", lastVisit: jan11_800,
   transType: histsvc.TRANSITION_LINK},

  
  {isInQuery: true, isVisit: true, isDetails: true, title: "taggariffic",
   uri: "http://foo.com/tagging/test.html", lastVisit: beginTime, isTag: true,
   tagArray: ["moz"] },

  
  
  {isInQuery: false, isVisit: true, isDetails: true, isPageAnnotation: true,
   uri: "http://www.foo.com/yiihah", annoName: goodAnnoName, annoVal: val,
   lastVisit: jan7_800, title: "moz"},

   
   {isInQuery: false, isVisit: true, isDetails: true,
    uri: "http://mail.foo.com/yiihah", title: "moz", lastVisit: jan6_815},

  
  {isInQuery: false, isVisit: true, isDetails: true, title: "moz",
   uri: "https://foo.com/", lastVisit: jan15_2045},

   
   {isInQuery: false, isVisit: true, isDetails: true,
    uri: "ftp://foo.com/ftp", lastVisit: jan12_1730,
    title: "hugelongconfmozlagurationofwordswithasearchtermsinit whoo-hoo"},

  
  {isInQuery: false, isVisit:true, isDetails: true, title: "moz",
   uri: "http://foo.com/tooearly.php", lastVisit: jan6_700},

  
  {isInQuery: false, isVisit:true, isDetails: true, isPageAnnotation: true,
   title: "moz", uri: "http://foo.com/badanno.htm", lastVisit: jan12_1730,
   annoName: badAnnoName, annoVal: val},
  
  
  {isInQuery: false, isVisit:true, isDetails: true, title: "changeme",
   uri: "http://foo.com/changeme1.htm", lastVisit: jan12_1730},

  
  {isInQuery: false, isVisit:true, isDetails: true, title: "changeme2",
   uri: "http://foo.com/changeme2.htm", lastVisit: jan7_800},

  
  {isInQuery: false, isVisit:true, isDetails: true, title: "moz",
   uri: "http://foo.com/changeme3.htm", lastVisit: dec27_800}];








function run_test() {

  
  populateDB(testData);

  
  var query = histsvc.getNewQuery();
  query.beginTime = beginTime;
  query.endTime = endTime;
  query.beginTimeReference = histsvc.TIME_RELATIVE_EPOCH;
  query.endTimeReference = histsvc.TIME_RELATIVE_EPOCH;
  query.searchTerms = "moz";
  query.uri = uri("http://foo.com");
  query.uriIsPrefix = true;
  query.annotation = "text/foo";
  query.annotationIsNot = true;

  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_URI_ASCENDING;
  options.resultType = options.RESULTS_AS_URI;
  
  
  

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  
  compareArrayToResult(testData, root);

  
  
  var addItem = [{isInQuery: true, isVisit: true, isDetails: true, title: "moz",
                 uri: "http://foo.com/i-am-added.html", lastVisit: jan11_800}];
  populateDB(addItem);
  LOG("Adding item foo.com/i-am-added.html");
  do_check_eq(isInResult(addItem, root), true);

  
  var change1 = [{isDetails: true, uri: "http://foo.com/changeme1",
                  lastVisit: jan12_1730, title: "moz moz mozzie"}];
  populateDB(change1);
  LOG("LiveUpdate by changing title");
  do_check_eq(isInResult(change1, root), true);

  
  
  
  
  
  
  





  
  var change3 = [{isDetails: true, uri: "http://foo.com/changeme3.htm",
                  title: "moz", lastVisit: jan15_2045}];
  populateDB(change3);
  LOG("LiveUpdate by adding visit within timerange");
  do_check_eq(isInResult(change3, root), true);

  
  
  





  
  var change5 = [{isDetails: true, uri: "http://foo.com/end.html", title: "deleted"}];
  populateDB(change5);
  LOG("LiveUpdate by deleting item by changing title");
  do_check_eq(isInResult(change5, root), false);

  
  
  var updateBatch = {
    runBatched: function (aUserData) {
      var batchChange = [{isDetails: true, uri: "http://foo.com/changeme2",
                          title: "moz", lastVisit: jan7_800},
                         {isPageAnnotation: true, uri: "http://foo.com/begin.html",
                          annoName: badAnnoName, annoVal: val}];
      populateDB(batchChange);
    }
  };

  histsvc.runInBatchMode(updateBatch, null);
  LOG("LiveUpdate by updating title in batch mode");
  do_check_eq(isInResult({uri: "http://foo.com/changeme2"}, root), true);

  LOG("LiveUpdate by deleting item by setting annotation in batch mode");
  do_check_eq(isInResult({uri: "http:/foo.com/begin.html"}, root), false);

  root.containerOpen = false;
}
