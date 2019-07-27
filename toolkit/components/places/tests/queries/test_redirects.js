





let visits = [];










function check_results_callback(aSequence) {
  
  do_check_eq(aSequence.length, 3);
  let includeHidden = aSequence[0];
  let maxResults = aSequence[1];
  let sortingMode = aSequence[2];
  print("\nTESTING: includeHidden(" + includeHidden + ")," +
                  " maxResults("    + maxResults    + ")," +
                  " sortingMode("   + sortingMode   + ").");

  function isHidden(aVisit) {
    return aVisit.transType == Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK ||
           aVisit.isRedirect;
  }

  
  let expectedData = visits.filter(function (aVisit, aIndex, aArray) {
    
    if (aVisit.transType == Ci.nsINavHistoryService.TRANSITION_EMBED)
      return false;

    if (!includeHidden && isHidden(aVisit)) {
      
      if (visits.filter(function (refVisit) {
        return refVisit.uri == aVisit.uri && !isHidden(refVisit);
          }).length == 0)
        return false;
    }

    return true;
  });

  
  let seen = [];
  expectedData = expectedData.filter(function (aData) {
    if (seen.indexOf(aData.uri) != -1)
      return false;
    else
      seen.push(aData.uri);
    return true;
  });

  
  function getFirstIndexFor(aEntry) {
    for (let i = 0; i < visits.length; i++) {
      if (visits[i].uri == aEntry.uri)
        return i;
    }
  }
  function comparator(a, b) {
    if (sortingMode == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING)
      return b.lastVisit - a.lastVisit;
    else if (sortingMode == Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING)
      return b.visitCount - a.visitCount;
    else
      return getFirstIndexFor(a) - getFirstIndexFor(b);
  }
  expectedData.sort(comparator);

  
  if (maxResults) {
    expectedData = expectedData.slice(0, maxResults);
  }

  
  let query = PlacesUtils.history.getNewQuery();
  let options = PlacesUtils.history.getNewQueryOptions();
  options.includeHidden = includeHidden;
  options.sortingMode = sortingMode;
  if (maxResults)
    options.maxResults = maxResults;

  
  let result = PlacesUtils.history.executeQuery(query, options);
  let root = result.root;
  root.containerOpen = true;
  compareArrayToResult(expectedData, root);
  root.containerOpen = false;
}
































function cartProd(aSequences, aCallback)
{
  if (aSequences.length === 0)
    return 0;

  
  
  let seqEltPtrs = aSequences.map(function (i) 0);

  let numProds = 0;
  let done = false;
  while (!done) {
    numProds++;

    
    let prod = [];
    for (let i = 0; i < aSequences.length; i++) {
      prod.push(aSequences[i][seqEltPtrs[i]]);
    }
    aCallback(prod);

    
    
    
    
    

    
    
    let seqPtr = aSequences.length - 1;
    while (!done) {
      
      seqEltPtrs[seqPtr]++;

      
      if (seqEltPtrs[seqPtr] >= aSequences[seqPtr].length) {
        seqEltPtrs[seqPtr] = 0;
        seqPtr--;

        
        if (seqPtr < 0)
          done = true;
      }
      else break;
    }
  }
  return numProds;
}

function run_test()
{
  run_next_test();
}






add_task(function test_add_visits_to_database()
{
  yield PlacesUtils.bookmarks.eraseEverything();

  
  let timeInMicroseconds = Date.now() * 1000;
  let visitCount = 1;

  
  let t = [
    Ci.nsINavHistoryService.TRANSITION_LINK,
    Ci.nsINavHistoryService.TRANSITION_TYPED,
    Ci.nsINavHistoryService.TRANSITION_BOOKMARK,
    
    
    
    Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK,
    
    
    
    
    
  ];

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: transition,
      uri: "http://" + transition + ".example.com/",
      title: transition + "-example",
      isRedirect: true,
      lastVisit: timeInMicroseconds--,
      visitCount: (transition == Ci.nsINavHistoryService.TRANSITION_EMBED ||
                   transition == Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK) ? 0 : visitCount++,
      isInQuery: true }));

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,
      uri: "http://" + transition + ".redirect.temp.example.com/",
      title: transition + "-redirect-temp-example",
      lastVisit: timeInMicroseconds--,
      isRedirect: true,
      referrer: "http://" + transition + ".example.com/",
      visitCount: visitCount++,
      isInQuery: true }));

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
      uri: "http://" + transition + ".redirect.perm.example.com/",
      title: transition + "-redirect-perm-example",
      lastVisit: timeInMicroseconds--,
      isRedirect: true,
      referrer: "http://" + transition + ".redirect.temp.example.com/",
      visitCount: visitCount++,
      isInQuery: true }));

  
  
  
  function getLastValue(aURI, aProperty) {
    for (let i = 0; i < visits.length; i++) {
      if (visits[i].uri == aURI) {
        return visits[i][aProperty];
      }
    }
    do_throw("Unknown uri.");
    return null;
  }
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
      uri: "http://" + transition + ".example.com/",
      title: getLastValue("http://" + transition + ".example.com/", "title"),
      lastVisit: getLastValue("http://" + transition + ".example.com/", "lastVisit"),
      isRedirect: true,
      referrer: "http://" + transition + ".redirect.perm.example.com/",
      visitCount: getLastValue("http://" + transition + ".example.com/", "visitCount"),
      isInQuery: true }));

  
  visits.push({ isBookmark: true,
    uri: "http://unvisited.bookmark.com/",
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    title: "Unvisited Bookmark",
    isInQuery: false });

  
  yield task_populateDB(visits);
});

add_task(function test_redirects()
{
  
  yield PlacesTestUtils.promiseAsyncUpdates();

  
  
  let includeHidden_options = [true, false];
  let maxResults_options = [5, 10, 20, null];
  
  
  let sorting_options = [Ci.nsINavHistoryQueryOptions.SORT_BY_NONE,
                         Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING,
                         Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING];
  
  cartProd([includeHidden_options, maxResults_options, sorting_options],
           check_results_callback);

  yield PlacesUtils.bookmarks.eraseEverything();

  yield PlacesTestUtils.clearHistory();
});
