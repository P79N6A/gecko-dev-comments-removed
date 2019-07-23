

















































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var tagging = Cc["@mozilla.org/browser/tagging-service;1"].
              getService(Ci.nsITaggingService);

var resultTypes = [
  {value: Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY, name: "RESULTS_AS_DATE_QUERY"},
  {value: Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY, name: "RESULTS_AS_SITE_QUERY"},
  {value: Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY, name: "RESULTS_AS_DATE_SITE_QUERY"},
  {value: Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY, name: "RESULTS_AS_TAG_QUERY"},
];

var sortingModes = [
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING, name: "SORT_BY_TITLE_ASCENDING"},
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_DESCENDING, name: "SORT_BY_TITLE_DESCENDING"},
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING, name: "SORT_BY_DATE_ASCENDING"},
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING, name: "SORT_BY_DATE_DESCENDING"},
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_ASCENDING, name: "SORT_BY_DATEADDED_ASCENDING"},
  {value: Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING, name: "SORT_BY_DATEADDED_DESCENDING"},
];



var pages = [
  "http://www.mozilla.org/c/",
  "http://www.mozilla.org/a/",
  "http://www.mozilla.org/b/",
  "http://www.mozilla.com/c/",
  "http://www.mozilla.com/a/",
  "http://www.mozilla.com/b/",
];

var tags = [
  "mozilla",
  "Development",
  "test",
];



































function cartProd(aSequences, aCallback)
{
  if (aSequences.length === 0)
    return 0;

  
  
  var seqEltPtrs = aSequences.map(function (i) 0);

  var numProds = 0;
  var done = false;
  while (!done) {
    numProds++;

    
    var prod = [];
    for (var i = 0; i < aSequences.length; i++) {
      prod.push(aSequences[i][seqEltPtrs[i]]);
    }
    aCallback(prod);

    
    
    
    
    

    
    
    var seqPtr = aSequences.length - 1;
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







function test_query_callback(aSequence) {
  do_check_eq(aSequence.length, 2);
  var resultType = aSequence[0];
  var sortingMode = aSequence[1];
  print("\n\n*** Testing default sorting for resultType (" + resultType.name + ") and sortingMode (" + sortingMode.name + ")");

  
  if (resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY &&
      (sortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING ||
       sortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING)) {
    
    sortingMode.value = Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
  }
  if (resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
      resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY ||
      resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY) {
    
    if (sortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_ASCENDING ||
       sortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING)
    sortingMode.value = Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
  }

  
  var query = histsvc.getNewQuery();
  var options = histsvc.getNewQueryOptions();
  options.resultType = resultType.value;
  options.sortingMode = sortingMode.value;

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  if (resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
      resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY) {
    
    check_children_sorting(root,
                           Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING);
  }
  else
    check_children_sorting(root, sortingMode.value);

  
  var container = root.getChild(0)
                      .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  container.containerOpen = true;

  if (resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY) {
    
    
    check_children_sorting(container,
                           Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING);
    
    
    
    let innerContainer = container.getChild(0)
                                  .QueryInterface(Ci.nsINavHistoryContainerResultNode);
    innerContainer.containerOpen = true;
    check_children_sorting(innerContainer,
                           Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING);
    innerContainer.containerOpen = false;
  }
  else if (resultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY) {
    
    
    check_children_sorting(container,
                           Ci.nsINavHistoryQueryOptions.SORT_BY_NONE);
  }
  else
    check_children_sorting(container, sortingMode.value);

  container.containerOpen = false;
  root.containerOpen = false;

  test_result_sortingMode_change(result, resultType, sortingMode);
}












function test_result_sortingMode_change(aResult, aResultType, aOriginalSortingMode) {
  var root = aResult.root;
  
  
  sortingModes.forEach(function sortingModeChecker(aForcedSortingMode) {
    print("\n* Test setting sortingMode (" + aForcedSortingMode.name + ") " +
          "on result with resultType (" + aResultType.name + ") " +
          "currently sorted as (" + aOriginalSortingMode.name + ")");

    aResult.sortingMode = aForcedSortingMode.value;
    root.containerOpen = true;

    if (aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
        aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY) {
      
      check_children_sorting(root,
                             Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING);
    }
    else if (aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY &&
             (aOriginalSortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING ||
              aOriginalSortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING)) {
      
      check_children_sorting(root,
                             Ci.nsINavHistoryQueryOptions.SORT_BY_NONE);
    }
    else
      check_children_sorting(root, aOriginalSortingMode.value);

    
    var container = root.getChild(0)
                        .QueryInterface(Ci.nsINavHistoryContainerResultNode);
    container.containerOpen = true;

    if (aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY) {
      
      
      check_children_sorting(container,
                             Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING);
      
      
      let innerContainer = container.getChild(0)
                                    .QueryInterface(Ci.nsINavHistoryContainerResultNode);
      innerContainer.containerOpen = true;
      check_children_sorting(innerContainer, aForcedSortingMode.value);
      innerContainer.containerOpen = false;
    }
    else {
      if (aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
          aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY ||
          aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY) {
        
        check_children_sorting(root, Ci.nsINavHistoryQueryOptions.SORT_BY_NONE);
      }
      else if (aResultType.value == Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY &&
             (aOriginalSortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING ||
              aOriginalSortingMode.value == Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING)) {
        
        check_children_sorting(root, Ci.nsINavHistoryQueryOptions.SORT_BY_NONE);
      }
      else
        check_children_sorting(root, aOriginalSortingMode.value);

      
      check_children_sorting(container, aForcedSortingMode.value);
    }

    container.containerOpen = false;
    root.containerOpen = false;
  });
}








function check_children_sorting(aRootNode, aExpectedSortingMode) {
  var results = [];
  print("Found children:");
  for (var i = 0; i < aRootNode.childCount; i++) {
    results[i] = aRootNode.getChild(i);
    print(i + " " + results[i].title);
  }

  
  function caseInsensitiveStringComparator(a, b) {
    var aLC = a.toLowerCase();
    var bLC = b.toLowerCase();
    if (aLC < bLC)
      return -1;
    if (aLC > bLC)
      return 1;
    return 0;
  }

  
  var comparator;
  switch(aExpectedSortingMode) {
    case Ci.nsINavHistoryQueryOptions.SORT_BY_NONE:
      comparator = function (a, b) {
        return 0;
      }
      break;
    case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING:
      comparator = function (a, b) {
        return caseInsensitiveStringComparator(a.title, b.title);
      }
      break;
    case Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_DESCENDING:
      comparator = function (a, b) {
        return -caseInsensitiveStringComparator(a.title, b.title);
      }
      break;
    case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING:
      comparator = function (a, b) {
        return a.time - b.time;
      }
      break;
    case Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING:
      comparator = function (a, b) {
        return b.time - a.time;
      }
    case Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_ASCENDING:
      comparator = function (a, b) {
        return a.dateAdded - b.dateAdded;
      }
      break;
    case Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING:
      comparator = function (a, b) {
        return b.dateAdded - a.dateAdded;
      }
      break;
    default:
      do_throw("Unknown sorting type: " + aExpectedSortingMode);
  }

  
  var sortedResults = results.slice();
  sortedResults.sort(comparator);
  
  for (var i = 0; i < sortedResults.length; i++) {
    if (sortedResults[i].title != results[i].title)
      print(i + " index wrong, expected " + sortedResults[i].title +
            " found " + results[i].title);
    do_check_eq(sortedResults[i].title, results[i].title);
  }
}




function run_test()
{
  
  var timeInMilliseconds = Date.now();
  var visitCount = 0;
  var dayOffset = 0;
  var visits = [];
  pages.forEach(function (aPageUrl) visits.push(
    { isVisit: true,
      isBookmark: true,
      transType: Ci.nsINavHistoryService.TRANSITION_TYPED,
      uri: aPageUrl,
      title: aPageUrl,
      
      lastVisit: (timeInMilliseconds - (18000 * 1000 * dayOffset++)) * 1000,
      visitCount: visitCount++,
      isTag: true,
      tagArray: tags,
      isInQuery: true }));
  populateDB(visits);

  cartProd([resultTypes, sortingModes], test_query_callback);

  
  pages.forEach(function(aPageUrl) tagging.untagURI(uri(aPageUrl), tags));
  remove_all_bookmarks();
  bh.removeAllPages();
}
