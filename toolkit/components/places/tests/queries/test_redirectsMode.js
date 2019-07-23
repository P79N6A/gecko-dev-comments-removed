







































let visits = [];










function check_results_callback(aSequence) {
  
  do_check_eq(aSequence.length, 3);
  let includeHidden = aSequence[0];
  let redirectsMode = aSequence[1];
  let maxResults = aSequence[2];

  print("\nTESTING: includeHidden(" + includeHidden + ")," +
                  " redirectsMode(" + redirectsMode + ")," +
                  " maxResults("    + maxResults    + ").");

  
  let expectedData = visits.filter(function(aVisit, aIndex, aArray) {
    switch (aVisit.transType) {
      case Ci.nsINavHistoryService.TRANSITION_TYPED:
      case Ci.nsINavHistoryService.TRANSITION_LINK:
      case Ci.nsINavHistoryService.TRANSITION_BOOKMARK:
      case Ci.nsINavHistoryService.TRANSITION_DOWNLOAD:
        return redirectsMode != Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET;
      case Ci.nsINavHistoryService.TRANSITION_EMBED:
        return includeHidden && redirectsMode != Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET;
      case Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY:
      case Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT:
        if (redirectsMode == Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_ALL)
          return true;
        if (redirectsMode == Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET) {
           
          return aArray.filter(function(refVisit) {
                                return refVisit.referrer == aVisit.uri;
                               }).length == 0;
        }
        return false;
      default:
        return false;
    }
  });

  
  if (maxResults) {
    expectedData = expectedData.filter(function(aVisit, aIndex) {
                                        return aIndex < maxResults;
                                       });
  }

  
  let query = histsvc.getNewQuery();
  let options = histsvc.getNewQueryOptions();
  options.includeHidden = includeHidden;
  options.redirectsMode = redirectsMode;
  if (maxResults)
    options.maxResults = maxResults;

  
  let result = histsvc.executeQuery(query, options);
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






function add_visits_to_database() {
  
  bhistsvc.removeAllPages();
  remove_all_bookmarks();

  
  let timeInMicroseconds = Date.now() * 1000;

  
  let t = [
    Ci.nsINavHistoryService.TRANSITION_LINK,
    Ci.nsINavHistoryService.TRANSITION_TYPED,
    Ci.nsINavHistoryService.TRANSITION_BOOKMARK,
    Ci.nsINavHistoryService.TRANSITION_EMBED,
    Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
  ];

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: transition,
      uri: "http://" + transition + ".example.com/",
      title: transition + "-example",
      lastVisit: timeInMicroseconds,
      isInQuery: true }));

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,
      uri: "http://" + transition + ".redirect.temp.example.com/",
      title: transition + "-redirect-temp-example",
      lastVisit: timeInMicroseconds,
      isRedirect: true,
      referrer: "http://" + transition + ".example.com/",
      isInQuery: true }));

  
  t.forEach(function (transition) visits.push(
    { isVisit: true,
      transType: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
      uri: "http://" + transition + ".redirect.perm.example.com/",
      title: transition + "-redirect-perm-example",
      lastVisit: timeInMicroseconds,
      isRedirect: true,
      referrer: "http://" + transition + ".redirect.temp.example.com/",
      isInQuery: true }));

  
  populateDB(visits);
}


function run_test() {
  
  add_visits_to_database();

  
  
  includeHidden_options = [true, false];
  redirectsMode_options =  [Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_ALL,
                            Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_SOURCE,
                            Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET];
  maxResults_options = [5, 10, null];
  
  cartProd([includeHidden_options, redirectsMode_options, maxResults_options],
           check_results_callback);

  
  bhistsvc.removeAllPages();
  remove_all_bookmarks();
}
