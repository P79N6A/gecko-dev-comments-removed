




































var testData = [];

function createTestData() {
  function generateVisits(aObj, aNum) {
    for(var i=0; i < aNum; i++)
      testData.push(aObj);
  }
  generateVisits({isInQuery: true, isDetails: true, title: "amo",
                 uri: "http://foo.com/", lastVisit: lastweek, isTag: true,
                 tagArray: ["moz"]}, 3);
  generateVisits({isInQuery: true, isDetails: true, isTag: true,
                 title: "bMoz", tagArray: ["bugzilla"], uri: "http://moilla.com/",
                 lastVisit: yesterday}, 5);
  generateVisits({isInQuery: true, isDetails: true, title: "C Moz",
                 uri: "http://foo.mail.com/changeme1.html"}, 7);
  generateVisits({isInQuery: false, isVisit: true, isTag: true, tagArray: ["moz"],
                 uri: "http://foo.change.co/changeme2.html"}, 1);
  generateVisits({isInQuery: false, isVisit: true, isDetails: true,
                 title: "zydeco", uri: "http://foo.com/changeme3.html"}, 5);
}

 


 function run_test() {
   createTestData();
   populateDB(testData);
   var query = histsvc.getNewQuery();
   query.searchTerms = "moz";
   query.minVisits = 2;

   
   var options = histsvc.getNewQueryOptions();
   options.sortingMode = options.SORT_BY_VISITCOUNT_ASCENDING;
   options.resultType = options.RESULTS_AS_VISIT;

   
   var result = histsvc.executeQuery(query, options);
   var root = result.root;
   root.containerOpen = true;

   LOG("Number of items in result set: " + root.childCount);
   for(var i=0; i < root.childCount; ++i) {
     LOG("result: " + root.getChild(i).uri + " Title: " + root.getChild(i).title);
   }

   
   compareArrayToResult(testData, root);

   
   
   LOG("Adding item to query")
   var tmp = [];
   for (var i=0; i < 2; i++) {
     tmp.push({isVisit: true, isDetails: true, uri: "http://foo.com/added.html",
              title: "ab moz"});
   }
   populateDB(tmp);
   for (var i=0; i < 2; i++)
     do_check_eq(root.getChild(i).title, "ab moz");

   
   LOG("Updating Item");
   var change2 = [{isVisit: true, isDetails: true, title: "moz",
                 uri: "http://foo.change.co/changeme2.html"}];
   populateDB(change2);
   do_check_true(isInResult(change2, root));

   
   
   LOG("Updating Items in batch");
   var updateBatch = {
     runBatched: function (aUserData) {
       batchchange = [{isDetails: true, uri: "http://foo.mail.com/changeme1.html",
                       title: "foo"},
                      {isTag: true, uri: "http://foo.com/changeme3.html",
                       tagArray: ["foo", "moz"]}];
       populateDB(batchchange);
     }
   };
   histsvc.runInBatchMode(updateBatch, null);
   do_check_false(isInResult({uri: "http://foo.mail.com/changeme1.html"}, root));
   do_check_true(isInResult({uri: "http://foo.com/changeme3.html"}, root));

   
   LOG("Delete item outside of batch");
   var change4 = [{isDetails: true, uri: "http://moilla.com/",
                   title: "mo,z"}];
   populateDB(change4);
   do_check_false(isInResult(change4, root));
}
