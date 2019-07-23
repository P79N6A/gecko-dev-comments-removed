





































var tests = [];



tests.push({
  _sortingMode: histsvc.SORT_BY_NONE,

  setup: function() {
    LOG("Sorting test 1: SORT BY NONE");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://urlB.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleB",
        keyword: "keywordB",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://urlA.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleA",
        keyword: "keywordA",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://urlC.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleC",
        keyword: "keywordC",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData;

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_TITLE_ASCENDING,

  setup: function() {
    LOG("Sorting test 2: SORT BY TITLE");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://urlB.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleB",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://urlA.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleA",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://urlC.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "titleC",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_TITLE_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_DATE_ASCENDING,

  setup: function() {
    LOG("Sorting test 3: SORT BY DATE");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds - 2,
        title: "I",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://is.com/",
        lastVisit: timeInMicroseconds - 1,
        title: "love",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://best.com/",
        lastVisit: timeInMicroseconds - 3,
        title: "moz",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData = [
      this._unsortedData[2],
      this._unsortedData[0],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();

    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_DATE_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_URI_ASCENDING,

  setup: function() {
    LOG("Sorting test 4: SORT BY URI");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://is.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "I",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://moz.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "love",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://best.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "moz",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData = [
      this._unsortedData[2],
      this._unsortedData[0],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_URI_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_VISITCOUNT_ASCENDING,

  setup: function() {
    LOG("Sorting test 5: SORT BY VISITCOUNT");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds,
        title: "I",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://is.com/",
        lastVisit: timeInMicroseconds,
        title: "love",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://best.com/",
        lastVisit: timeInMicroseconds,
        title: "moz",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData = [
      this._unsortedData[0],
      this._unsortedData[2],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
    
    histsvc.addVisit(uri("http://is.com/"), timeInMicroseconds, null,
                     histsvc.TRANSITION_TYPED, false, 0);
    histsvc.addVisit(uri("http://is.com/"), timeInMicroseconds, null,
                     histsvc.TRANSITION_TYPED, false, 0);
    histsvc.addVisit(uri("http://best.com/"), timeInMicroseconds, null,
                     histsvc.TRANSITION_TYPED, false, 0);                     
  },

  check: function() {
    
    var query = histsvc.getNewQuery();

    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_VISITCOUNT_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_KEYWORD_ASCENDING,

  setup: function() {
    LOG("Sorting test 6: SORT BY KEYWORD");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://moz.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "I",
        keyword: "a",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://is.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "love",
        keyword: "c",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://best.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "moz",
        keyword: "b",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[0],
      this._unsortedData[2],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_KEYWORD_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_DATEADDED_ASCENDING,

  setup: function() {
    LOG("Sorting test 7: SORT BY DATEADDED");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        uri: "http://moz.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "I",
        dateAdded: timeInMicroseconds -1,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://is.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "love",
        dateAdded: timeInMicroseconds - 2,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://best.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "moz",
        dateAdded: timeInMicroseconds,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_DATEADDED_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: histsvc.SORT_BY_LASTMODIFIED_ASCENDING,

  setup: function() {
    LOG("Sorting test 8: SORT BY LASTMODIFIED");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        uri: "http://moz.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "I",
        lastModified: timeInMicroseconds -1,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://is.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "love",
        lastModified: timeInMicroseconds - 2,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://best.com/",
        parentFolder: bmsvc.toolbarFolder,
        index: bmsvc.DEFAULT_INDEX,
        title: "moz",
        lastModified: timeInMicroseconds,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_LASTMODIFIED_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});










tests.push({
  _sortingMode: histsvc.SORT_BY_ANNOTATION_ASCENDING,

  setup: function() {
    LOG("Sorting test 10: SORT BY ANNOTATION");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds,
        title: "I",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 2,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://is.com/",
        lastVisit: timeInMicroseconds,
        title: "love",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 1,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        uri: "http://best.com/",
        lastVisit: timeInMicroseconds,
        title: "moz",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 3,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);                  
  },

  check: function() {
    
    var query = histsvc.getNewQuery();

    
    var options = histsvc.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = histsvc.SORT_BY_ANNOTATION_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



function prepare_for_next_test() {
  
  bhistsvc.removeAllPages();
  remove_all_bookmarks();
}






function run_test() {
  prepare_for_next_test();
  while (tests.length) {
    let test = tests.shift();
    test.setup();
    test.check();
    
    test.check_reverse();
    prepare_for_next_test();
  }
}
