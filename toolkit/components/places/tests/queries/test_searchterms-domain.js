





































 
 
 
 var testData = [
   
   {isInQuery: true, isVisit: true, isDetails: true,
    uri: "ftp://foo.com/ftp", lastVisit: lastweek,
    title: "hugelongconfmozlagurationofwordswithasearchtermsinit whoo-hoo"},

   
   {isInQuery: true, isVisit: true, isDetails: true, isPageAnnotation: true,
    uri: "http://foo.com/", annoName: "moz/test", annoVal: "val",
    lastVisit: lastweek, title: "you know, moz is cool"},

   
   {isInQuery: true, isVisit: true, isDetails: true, title: "amozzie",
    isRedirect: true, uri: "http://mail.foo.com/redirect", lastVisit: old,
    referrer: "http://myreferrer.com", transType: histsvc.TRANSITION_LINK},

   
   {isInQuery: true, isVisit: true, isDetails: true,
    uri: "http://mail.foo.com/yiihah", title: "blahmoz", lastVisit: daybefore},

   
   {isInQuery: true, isVisit: true, isDetails: true, isTag: true,
    uri: "http://www.foo.com/yiihah", tagArray: ["moz"],
    lastVisit: yesterday, title: "foo"},

   
   {isInQuery: true, isVisit: true, isDetails: true, title: "moz",
    uri: "https://foo.com/", lastVisit: today},

   
   {isInQuery: false, isVisit:true, isDetails: true, title: "m o z",
    uri: "http://foo.com/tooearly.php", lastVisit: today},

   
   {isInQuery: false, isVisit:true, isDetails: true, title: "moz",
    uri: "http://sffoo.com/justwrong.htm", lastVisit: tomorrow},

   
   {isInQuery: false, isVisit:true, isDetails: true, title: "m%0o%0z",
    uri: "http://foo.com/changeme1.htm", lastVisit: yesterday},

   
   {isInQuery: false, isVisit:true, isDetails: true, title: "m,oz",
    uri: "http://foo.com/changeme2.htm", lastVisit: tomorrow}];




function run_test() {
  populateDB(testData);
  var query = histsvc.getNewQuery();
  query.searchTerms = "moz";
  query.domain = "foo.com";
  query.domainIsHost = false;

  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_ASCENDING;
  options.resultType = options.RESULTS_AS_URI;

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  LOG("Number of items in result set: " + root.childCount);
  for(var i=0; i < root.childCount; ++i) {
    LOG("result: " + root.getChild(i).uri + " Title: " + root.getChild(i).title);
  }

  
  compareArrayToResult(testData, root);

  
  
  LOG("Adding item to query")
  var change1 = [{isVisit: true, isDetails: true, uri: "http://foo.com/added.htm",
                  title: "moz", transType: histsvc.TRANSITION_LINK}];
  populateDB(change1);
  do_check_true(isInResult(change1, root));

  
  LOG("Updating Item");
  var change2 = [{isDetails: true, uri: "http://foo.com/changeme1.htm",
                  title: "moz" }];
  populateDB(change2);
  do_check_true(isInResult(change2, root));
                  
  
  
  LOG("Updating Items in batch");
  var updateBatch = {
    runBatched: function (aUserData) {
      var batchchange = [{isDetails: true, uri:"http://foo.com/changeme2.htm",
                          title: "moz"},
                         {isDetails: true, uri: "http://mail.foo.com/yiihah",
                          title: "moz now updated"},
                         {isDetails: true, uri: "ftp://foo.com/ftp", title: "gone"}];
      populateDB(batchchange);
    }
  };
  histsvc.runInBatchMode(updateBatch, null);
  do_check_true(isInResult({uri: "http://foo.com/changeme2.htm"}, root));
  do_check_true(isInResult({uri: "http://mail.foo.com/yiihah"}, root));
  do_check_false(isInResult({uri: "ftp://foo.com/ftp"}, root));

  
  LOG("Delete item outside of batch");
  var change4 = [{isDetails: true, uri: "https://foo.com/",
                  title: "mo,z"}];
  populateDB(change4);
  do_check_false(isInResult(change4, root));
}
