




































var testData = [];
var now = Date.now() * 1000;
function createTestData() {
  function generateVisits(aPage) {
    for (var i = 0; i < aPage.visitCount; i++) {
      testData.push({ isInQuery: aPage.inQuery,
                      isVisit: true,
                      title: aPage.title,
                      uri: aPage.uri,
                      lastVisit: now++,
                      isTag: aPage.tags && aPage.tags.length > 0,
                      tagArray: aPage.tags });
    }
  }
  
  var pages = [
    { uri: "http://foo.com/", title: "amo", tags: ["moz"], visitCount: 3, inQuery: true },
    { uri: "http://moilla.com/", title: "bMoz", tags: ["bugzilla"], visitCount: 5, inQuery: true },
    { uri: "http://foo.mail.com/changeme1.html", title: "c Moz", visitCount: 7, inQuery: true },
    { uri: "http://foo.mail.com/changeme2.html", tags: ["moz"], title: "", visitCount: 1, inQuery: false },
    { uri: "http://foo.mail.com/changeme3.html", title: "zydeco", visitCount: 5, inQuery: false },
  ];
  pages.forEach(generateVisits);
}

 


 function run_test() {
   
   return;

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
     tmp.push({ isVisit: true,
                uri: "http://foo.com/added.html",
                title: "ab moz" });
   }
   populateDB(tmp);
   for (var i=0; i < 2; i++)
     do_check_eq(root.getChild(i).title, "ab moz");

   
   LOG("Updating Item");
   var change2 = [{ isVisit: true,
                    title: "moz",
                    uri: "http://foo.mail.com/changeme2.html" }];
   populateDB(change2);
   do_check_true(isInResult(change2, root));

   
   
   LOG("Updating Items in batch");
   var updateBatch = {
     runBatched: function (aUserData) {
       batchchange = [{ isVisit: true,
                        lastVisit: now++,
                        uri: "http://foo.mail.com/changeme1.html",
                        title: "foo"},
                      { isVisit: true,
                        lastVisit: now++,
                        uri: "http://foo.mail.com/changeme3.html",
                        title: "moz",
                        isTag: true,
                        tagArray: ["foo", "moz"] }];
       populateDB(batchchange);
     }
   };
   histsvc.runInBatchMode(updateBatch, null);
   do_check_false(isInResult({uri: "http://foo.mail.com/changeme1.html"}, root));
   do_check_true(isInResult({uri: "http://foo.mail.com/changeme3.html"}, root));

   
   LOG("Delete item outside of batch");
   var change4 = [{ isVisit: true,
                    lastVisit: now++,
                    uri: "http://moilla.com/",
                    title: "mo,z" }];
   populateDB(change4);
   do_check_false(isInResult(change4, root));
}
