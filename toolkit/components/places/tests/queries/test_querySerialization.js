































































































const CHOOSE_HOW_MANY_SWITCHES_LO = 1;
const CHOOSE_HOW_MANY_SWITCHES_HI = 2;

const NUM_MULTIPLE_QUERIES        = 2;
























const querySwitches = [
  
  {
    
    
    
    flag:        "hasBeginTime",
    subswitches: ["beginTime", "beginTimeReference", "absoluteBeginTime"],
    desc:        "nsINavHistoryQuery.hasBeginTime",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.beginTime = Date.now() * 1000;
        aQuery.beginTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_EPOCH;
      },
      function (aQuery, aQueryOptions) {
        aQuery.beginTime = Date.now() * 1000;
        aQuery.beginTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_TODAY;
      }
    ]
  },
  
  {
    flag:        "hasEndTime",
    subswitches: ["endTime", "endTimeReference", "absoluteEndTime"],
    desc:        "nsINavHistoryQuery.hasEndTime",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.endTime = Date.now() * 1000;
        aQuery.endTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_EPOCH;
      },
      function (aQuery, aQueryOptions) {
        aQuery.endTime = Date.now() * 1000;
        aQuery.endTimeReference = Ci.nsINavHistoryQuery.TIME_RELATIVE_TODAY;
      }
    ]
  },
  
  {
    flag:        "hasSearchTerms",
    subswitches: ["searchTerms"],
    desc:        "nsINavHistoryQuery.hasSearchTerms",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.searchTerms = "shrimp and white wine";
      },
      function (aQuery, aQueryOptions) {
        aQuery.searchTerms = "";
      }
    ]
  },
  
  {
    flag:        "hasDomain",
    subswitches: ["domain", "domainIsHost"],
    desc:        "nsINavHistoryQuery.hasDomain",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.domain = "mozilla.com";
        aQuery.domainIsHost = false;
      },
      function (aQuery, aQueryOptions) {
        aQuery.domain = "www.mozilla.com";
        aQuery.domainIsHost = true;
      },
      function (aQuery, aQueryOptions) {
        aQuery.domain = "";
      }
    ]
  },
  
  {
    flag:        "hasUri",
    subswitches: ["uri", "uriIsPrefix"],
    desc:        "nsINavHistoryQuery.hasUri",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.uri = uri("http://mozilla.com");
        aQuery.uriIsPrefix = false;
      },
      function (aQuery, aQueryOptions) {
        aQuery.uri = uri("http://mozilla.com");
        aQuery.uriIsPrefix = true;
      }
    ]
  },
  
  {
    flag:        "hasAnnotation",
    subswitches: ["annotation", "annotationIsNot"],
    desc:        "nsINavHistoryQuery.hasAnnotation",
    matches:     flagSwitchMatches,
    runs:        [
      function (aQuery, aQueryOptions) {
        aQuery.annotation = "bookmarks/toolbarFolder";
        aQuery.annotationIsNot = false;
      },
      function (aQuery, aQueryOptions) {
        aQuery.annotation = "bookmarks/toolbarFolder";
        aQuery.annotationIsNot = true;
      }
    ]
  },
  
  {
    
    property: "minVisits",
    desc:     "nsINavHistoryQuery.minVisits",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQuery.minVisits = 0x7fffffff; 
      }
    ]
  },
  
  {
    property: "maxVisits",
    desc:     "nsINavHistoryQuery.maxVisits",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQuery.maxVisits = 0x7fffffff; 
      }
    ]
  },
  
  {
    property: "onlyBookmarked",
    desc:     "nsINavHistoryQuery.onlyBookmarked",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQuery.onlyBookmarked = true;
      }
    ]
  },
  
  {
    desc:    "nsINavHistoryQuery.getFolders",
    matches: function (aQuery1, aQuery2) {
      var q1Folders = aQuery1.getFolders();
      var q2Folders = aQuery2.getFolders();
      if (q1Folders.length !== q2Folders.length)
        return false;
      for (let i = 0; i < q1Folders.length; i++) {
        if (q2Folders.indexOf(q1Folders[i]) < 0)
          return false;
      }
      for (let i = 0; i < q2Folders.length; i++) {
        if (q1Folders.indexOf(q2Folders[i]) < 0)
          return false;
      }
      return true;
    },
    runs: [
      function (aQuery, aQueryOptions) {
        aQuery.setFolders([], 0);
      },
      function (aQuery, aQueryOptions) {
        aQuery.setFolders([bmsvc.placesRoot], 1);
      },
      function (aQuery, aQueryOptions) {
        aQuery.setFolders([bmsvc.placesRoot, bmsvc.tagsFolder], 2);
      }
    ]
  },
  
  {
    desc:    "nsINavHistoryQuery.getTags",
    matches: function (aQuery1, aQuery2) {
      if (aQuery1.tagsAreNot !== aQuery2.tagsAreNot)
        return false;
      var q1Tags = aQuery1.tags;
      var q2Tags = aQuery2.tags;
      if (q1Tags.length !== q2Tags.length)
        return false;
      for (let i = 0; i < q1Tags.length; i++) {
        if (q2Tags.indexOf(q1Tags[i]) < 0)
          return false;
      }
      for (let i = 0; i < q2Tags.length; i++) {
        if (q1Tags.indexOf(q2Tags[i]) < 0)
          return false;
      }
      return true;
    },
    runs: [
      function (aQuery, aQueryOptions) {
        aQuery.tags = [];
      },
      function (aQuery, aQueryOptions) {
        aQuery.tags = [""];
      },
      function (aQuery, aQueryOptions) {
        aQuery.tags = [
          "foo",
          "七難",
          "",
          "いっぱいおっぱい",
          "Abracadabra",
          "１２３",
          "Here's a pretty long tag name with some = signs and 1 2 3s and spaces oh jeez will it work I hope so!",
          "アスキーでございません",
          "あいうえお",
        ];
      },
      function (aQuery, aQueryOptions) {
        aQuery.tags = [
          "foo",
          "七難",
          "",
          "いっぱいおっぱい",
          "Abracadabra",
          "１２３",
          "Here's a pretty long tag name with some = signs and 1 2 3s and spaces oh jeez will it work I hope so!",
          "アスキーでございません",
          "あいうえお",
        ];
        aQuery.tagsAreNot =  true;
      }
    ]
  },
];


const queryOptionSwitches = [
  
  {
    desc:    "nsINavHistoryQueryOptions.sortingMode",
    matches: function (aOptions1, aOptions2) {
      if (aOptions1.sortingMode === aOptions2.sortingMode) {
        switch (aOptions1.sortingMode) {
          case aOptions1.SORT_BY_ANNOTATION_ASCENDING:
          case aOptions1.SORT_BY_ANNOTATION_DESCENDING:
            return aOptions1.sortingAnnotation === aOptions2.sortingAnnotation;
            break;
        }
        return true;
      }
      return false;
    },
    runs: [
      function (aQuery, aQueryOptions) {
        aQueryOptions.sortingMode = aQueryOptions.SORT_BY_DATE_ASCENDING;
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.sortingMode = aQueryOptions.SORT_BY_ANNOTATION_ASCENDING;
        aQueryOptions.sortingAnnotation = "bookmarks/toolbarFolder";
      }
    ]
  },
  
  {
    
    property: "resultType",
    desc:     "nsINavHistoryQueryOptions.resultType",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.resultType = aQueryOptions.RESULTS_AS_URI;
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.resultType = aQueryOptions.RESULTS_AS_FULL_VISIT;
      }
    ]
  },
  
  {
    property: "excludeItems",
    desc:     "nsINavHistoryQueryOptions.excludeItems",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.excludeItems = true;
      }
    ]
  },
  
  {
    property: "excludeQueries",
    desc:     "nsINavHistoryQueryOptions.excludeQueries",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.excludeQueries = true;
      }
    ]
  },
  
  {
    property: "excludeReadOnlyFolders",
    desc:     "nsINavHistoryQueryOptions.excludeReadOnlyFolders",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.excludeReadOnlyFolders = true;
      }
    ]
  },
  
  {
    property: "excludeItemIfParentHasAnnotation",
    desc:     "nsINavHistoryQueryOptions.excludeItemIfParentHasAnnotation",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.excludeItemIfParentHasAnnotation =
          "bookmarks/toolbarFolder";
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.excludeItemIfParentHasAnnotation = "";
      }
    ]
  },
  
  {
    property: "expandQueries",
    desc:     "nsINavHistoryQueryOptions.expandQueries",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.expandQueries = true;
      }
    ]
  },
  
  {
    property: "includeHidden",
    desc:     "nsINavHistoryQueryOptions.includeHidden",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.includeHidden = true;
      }
    ]
  },
  
  {
    property: "maxResults",
    desc:     "nsINavHistoryQueryOptions.maxResults",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.maxResults = 0xffffffff; 
      }
    ]
  },
  
  {
    property: "queryType",
    desc:     "nsINavHistoryQueryOptions.queryType",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.queryType = aQueryOptions.QUERY_TYPE_HISTORY;
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.queryType = aQueryOptions.QUERY_TYPE_UNIFIED;
      }
    ]
  },
  
  {
    property: "redirectsMode",
    desc:     "nsINavHistoryQueryOptions.redirectsMode",
    matches:  simplePropertyMatches,
    runs:     [
      function (aQuery, aQueryOptions) {
        aQueryOptions.redirectsMode = aQueryOptions.REDIRECTS_MODE_ALL;
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.redirectsMode = aQueryOptions.REDIRECTS_MODE_TARGET;
      },
      function (aQuery, aQueryOptions) {
        aQueryOptions.redirectsMode = aQueryOptions.REDIRECTS_MODE_SOURCE;
      }
    ]
  },
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


















function choose(aSet, aHowMany, aCallback)
{
  
  var ptrs = [];
  for (let i = 0; i < aHowMany; i++) {
    ptrs.push(i);
  }

  var numFound = 0;
  var done = false;
  while (!done) {
    numFound++;
    aCallback(ptrs.map(function (p) aSet[p]));

    
    
    
    
    
    

    
    ptrs[ptrs.length - 1]++;

    
    if (ptrs[ptrs.length - 1] >= aSet.length) {
      
      let si = aSet.length - 2; 
      let pi = ptrs.length - 2; 
      while (pi >= 0 && ptrs[pi] === si) {
        pi--;
        si--;
      }

      
      if (pi < 0)
        done = true;
      else {
        
        
        
        ptrs[pi]++;
        for (let i = 0; i < ptrs.length - pi - 1; i++) {
          ptrs[i + pi + 1] = ptrs[pi] + i + 1;
        }
      }
    }
  }
  return numFound;
}











function flagSwitchMatches(aQuery1, aQuery2)
{
  if (aQuery1[this.flag] && aQuery2[this.flag]) {
    for (let p in this.subswitches) {
      if (p in aQuery1 && p in aQuery2) {
        if (aQuery1[p] instanceof Ci.nsIURI) {
          if (!aQuery1[p].equals(aQuery2[p]))
            return false;
        }
        else if (aQuery1[p] !== aQuery2[p])
          return false;
      }
    }
  }
  else if (aQuery1[this.flag] || aQuery2[this.flag])
    return false;

  return true;
}
















function queryObjsEqual(aSwitches, aObj1, aObj2)
{
  for (let i = 0; i < aSwitches.length; i++) {
    if (!aSwitches[i].matches(aObj1, aObj2))
      return false;
  }
  return true;
}









function runQuerySequences(aHowManyLo, aHowManyHi)
{
  var allSwitches = querySwitches.concat(queryOptionSwitches);
  var prevQueries = [];
  var prevOpts = [];

  
  for (let howMany = aHowManyLo; howMany <= aHowManyHi; howMany++) {
    let numIters = 0;
    print("CHOOSING " + howMany + " SWITCHES");

    
    choose(allSwitches, howMany, function (chosenSwitches) {
      print(numIters);
      numIters++;

      
      
      var runs = chosenSwitches.map(function (s) {
        if (s.desc)
          print("  " + s.desc);
        return s.runs;
      });

      
      
      
      
      
      
      
      
      
      cartProd(runs, function (runSet) {
        
        var query = histsvc.getNewQuery();
        var opts = histsvc.getNewQueryOptions();
        for (let i = 0; i < runSet.length; i++) {
          runSet[i](query, opts);
        }
        serializeDeserialize([query], opts);

        
        prevQueries.push(query);
        prevOpts.push(opts);
        if (prevQueries.length >= NUM_MULTIPLE_QUERIES) {
          
          
          
          for (let i = 0; i < prevOpts.length; i++) {
            serializeDeserialize(prevQueries, prevOpts[i]);
          }
          prevQueries.shift();
          prevOpts.shift();
        }
      });
    });
  }
  print("\n");
}












function serializeDeserialize(aQueryArr, aQueryOptions)
{
  var queryStr = histsvc.queriesToQueryString(aQueryArr,
                                              aQueryArr.length,
                                              aQueryOptions);
  print("  " + queryStr);
  var queryArr2 = {};
  var opts2 = {};
  histsvc.queryStringToQueries(queryStr, queryArr2, {}, opts2);
  queryArr2 = queryArr2.value;
  opts2 = opts2.value;

  
  do_check_eq(aQueryArr.length, queryArr2.length);

  
  
  
  
  
  
  
  for (let i = 0; i < aQueryArr.length; i++) {
    let j = 0;
    for (; j < queryArr2.length; j++) {
      if (queryObjsEqual(querySwitches, aQueryArr[i], queryArr2[j]))
        break;
    }
    if (j < queryArr2.length)
      queryArr2.splice(j, 1);
  }
  do_check_eq(queryArr2.length, 0);

  
  do_check_true(queryObjsEqual(queryOptionSwitches, aQueryOptions, opts2));
}











function simplePropertyMatches(aObj1, aObj2)
{
  return aObj1[this.property] === aObj2[this.property];
}



function run_test()
{
  runQuerySequences(CHOOSE_HOW_MANY_SWITCHES_LO, CHOOSE_HOW_MANY_SWITCHES_HI);
}
