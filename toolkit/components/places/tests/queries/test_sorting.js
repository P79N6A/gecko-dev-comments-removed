






































var tests = [];



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_NONE,

  setup: function() {
    LOG("Sorting test 1: SORT BY NONE");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/b",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "y",
        keyword: "b",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "z",
        keyword: "a",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "x",
        keyword: "c",
        isInQuery: true },
    ];

    this._sortedData = this._unsortedData;

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_ASCENDING,

  setup: function() {
    LOG("Sorting test 2: SORT BY TITLE");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/b1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "y",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "z",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "x",
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "y",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[2],
      this._unsortedData[0],
      this._unsortedData[3],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_TITLE_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_ASCENDING,

  setup: function() {
    LOG("Sorting test 3: SORT BY DATE");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        isBookmark: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 0,
        uri: "http://example.com/c1",
        lastVisit: timeInMicroseconds - 2,
        title: "x1",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        isBookmark: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 1,
        uri: "http://example.com/a",
        lastVisit: timeInMicroseconds - 1,
        title: "z",
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        isBookmark: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 2,
        uri: "http://example.com/b",
        lastVisit: timeInMicroseconds - 3,
        title: "y",
        isInQuery: true },

      
      { isVisit: true,
        isDetails: true,
        isBookmark: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 3,
        uri: "http://example.com/c2",
        lastVisit: timeInMicroseconds - 2,
        title: "x2",
        isInQuery: true },

      
      { isVisit: true,
        isDetails: true,
        isBookmark: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 4,
        uri: "http://example.com/c2",
        lastVisit: timeInMicroseconds - 2,
        title: "x2",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[2],
      this._unsortedData[0],
      this._unsortedData[3],
      this._unsortedData[4],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_URI_ASCENDING,

  setup: function() {
    LOG("Sorting test 4: SORT BY URI");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        isDetails: true,
        lastVisit: timeInMicroseconds,
        uri: "http://example.com/b",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 0,
        title: "y",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 1,
        title: "x",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 2,
        title: "z",
        isInQuery: true },

      
      { isBookmark: true,
        isDetails: true,
        lastVisit: timeInMicroseconds + 1,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 3,
        title: "x",
        isInQuery: true },

      
      { isFolder: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 4,
        title: "a",
        isInQuery: true },

      
      { isBookmark: true,
        isDetails: true,
        lastVisit: timeInMicroseconds + 1,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 5,
        title: "x",
        isInQuery: true },

      
      { isFolder: true,
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 6,
        title: "a",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[4],
      this._unsortedData[6],
      this._unsortedData[2],
      this._unsortedData[0],
      this._unsortedData[1],
      this._unsortedData[3],
      this._unsortedData[5],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_URI_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_ASCENDING,

  setup: function() {
    LOG("Sorting test 5: SORT BY VISITCOUNT");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/a",
        lastVisit: timeInMicroseconds,
        title: "z",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 0,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        lastVisit: timeInMicroseconds,
        title: "x",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 1,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/b1",
        lastVisit: timeInMicroseconds,
        title: "y1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 2,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        lastVisit: timeInMicroseconds + 1,
        title: "y2a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 3,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        lastVisit: timeInMicroseconds + 1,
        title: "y2b",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 4,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[0],
      this._unsortedData[2],
      this._unsortedData[3],
      this._unsortedData[4],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
    
    PlacesUtils.history.addVisit(uri("http://example.com/a"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/b1"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/b1"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/b2"), timeInMicroseconds + 1, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/b2"), timeInMicroseconds + 1, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/c"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/c"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
    PlacesUtils.history.addVisit(uri("http://example.com/c"), timeInMicroseconds, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_KEYWORD_ASCENDING,

  setup: function() {
    LOG("Sorting test 6: SORT BY KEYWORD");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "z",
        keyword: "a",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "x",
        keyword: "c",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/b1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "y9",
        keyword: "b",
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/null2",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "null8",
        keyword: null,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/null1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "null9",
        keyword: null,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "y8",
        keyword: "b",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[3],
      this._unsortedData[4],
      this._unsortedData[0],
      this._unsortedData[5],
      this._unsortedData[2],
      this._unsortedData[1],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_KEYWORD_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_ASCENDING,

  setup: function() {
    LOG("Sorting test 7: SORT BY DATEADDED");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/b1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 0,
        title: "y1",
        dateAdded: timeInMicroseconds -1,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 1,
        title: "z",
        dateAdded: timeInMicroseconds - 2,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 2,
        title: "x",
        dateAdded: timeInMicroseconds,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 3,
        title: "y2",
        dateAdded: timeInMicroseconds - 1,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b3",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 4,
        title: "y3",
        dateAdded: timeInMicroseconds - 1,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[3],
      this._unsortedData[4],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_ASCENDING,

  setup: function() {
    LOG("Sorting test 8: SORT BY LASTMODIFIED");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isBookmark: true,
        uri: "http://example.com/b1",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 0,
        title: "y1",
        lastModified: timeInMicroseconds -1,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/a",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 1,
        title: "z",
        lastModified: timeInMicroseconds - 2,
        isInQuery: true },

      { isBookmark: true,
        uri: "http://example.com/c",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 2,
        title: "x",
        lastModified: timeInMicroseconds,
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://example.com/b2",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 3,
        title: "y2",
        lastModified: timeInMicroseconds - 1,
        isInQuery: true },

      
      
      { isBookmark: true,
        uri: "http://example.com/b3",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: 4,
        title: "y3",
        lastModified: timeInMicroseconds - 1,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[3],
      this._unsortedData[4],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_TAGS_ASCENDING,

  setup: function() {
    LOG("Sorting test 9: SORT BY TAGS");

    this._unsortedData = [
      { isBookmark: true,
        uri: "http://url2.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title x",
        isTag: true,
        tagArray: ["x", "y", "z"],
        isInQuery: true },

      { isBookmark: true,
        uri: "http://url1a.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title y1",
        isTag: true,
        tagArray: ["a", "b"],
        isInQuery: true },

      { isBookmark: true,
        uri: "http://url3a.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title w1",
        isInQuery: true },

      { isBookmark: true,
        uri: "http://url0.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title z",
        isTag: true,
        tagArray: ["a", "y", "z"],
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://url1b.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title y2",
        isTag: true,
        tagArray: ["b", "a"],
        isInQuery: true },

      
      { isBookmark: true,
        uri: "http://url3b.com/",
        parentFolder: PlacesUtils.bookmarks.toolbarFolder,
        index: PlacesUtils.bookmarks.DEFAULT_INDEX,
        title: "title w2",
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[2],
      this._unsortedData[5],
      this._unsortedData[1],
      this._unsortedData[4],
      this._unsortedData[3],
      this._unsortedData[0],
    ];

    
    populateDB(this._unsortedData);
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
    query.onlyBookmarked = true;
    
    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_TAGS_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});




tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING,

  setup: function() {
    LOG("Sorting test 10: SORT BY ANNOTATION (int32)");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        lastVisit: timeInMicroseconds,
        uri: "http://example.com/b1",
        title: "y1",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 2,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        lastVisit: timeInMicroseconds,
        uri: "http://example.com/a",
        title: "z",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 1,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },

      { isVisit: true,
        isDetails: true,
        lastVisit: timeInMicroseconds,
        uri: "http://example.com/c",
        title: "x",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 3,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },

      
      { isVisit: true,
        isDetails: true,
        lastVisit: timeInMicroseconds,
        uri: "http://example.com/b2",
        title: "y2",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 2,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
        isInQuery: true },
    ];

    this._sortedData = [
      this._unsortedData[1],
      this._unsortedData[0],
      this._unsortedData[3],
      this._unsortedData[2],
    ];

    
    populateDB(this._unsortedData);                  
  },

  check: function() {
    
    var query = PlacesUtils.history.getNewQuery();

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingAnnotation = "sorting";
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});




tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING,

  setup: function() {
    LOG("Sorting test 11: SORT BY ANNOTATION (int64)");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds,
        title: "I",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 0xffffffff1,
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
        annoVal: 0xffffffff0,
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
        annoVal: 0xffffffff2,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
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
    
    var query = PlacesUtils.history.getNewQuery();

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingAnnotation = "sorting";
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});




tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING,

  setup: function() {
    LOG("Sorting test 12: SORT BY ANNOTATION (string)");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds,
        title: "I",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: "a",
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
        annoVal: "",
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
        annoVal: "z",
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
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
    
    var query = PlacesUtils.history.getNewQuery();

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingAnnotation = "sorting";
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});




tests.push({
  _sortingMode: Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_ASCENDING,

  setup: function() {
    LOG("Sorting test 13: SORT BY ANNOTATION (double)");

    var timeInMicroseconds = Date.now() * 1000;
    this._unsortedData = [
      { isVisit: true,
        isDetails: true,
        uri: "http://moz.com/",
        lastVisit: timeInMicroseconds,
        title: "I",
        isPageAnnotation: true,
        annoName: "sorting",
        annoVal: 1.2,
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
        annoVal: 1.1,
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
        annoVal: 1.3,
        annoFlags: 0,
        annoExpiration: Ci.nsIAnnotationService.EXPIRE_NEVER,
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
    
    var query = PlacesUtils.history.getNewQuery();

    
    var options = PlacesUtils.history.getNewQueryOptions();
    options.sortingAnnotation = "sorting";
    options.sortingMode = this._sortingMode;

    
    var result = PlacesUtils.history.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    compareArrayToResult(this._sortedData, root);
    root.containerOpen = false;
  },

  check_reverse: function() {
    this._sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_ANNOTATION_DESCENDING;
    this._sortedData.reverse();
    this.check();
  }
});



function prepare_and_run_next_test(aTest) {
  aTest.setup();
  aTest.check();
  
  aTest.check_reverse();
  
  remove_all_bookmarks();
  waitForClearHistory(runNextTest);
}






function run_test() {
  do_test_pending();
  runNextTest();
}

function runNextTest() {
  if (tests.length) {
    let test = tests.shift();
    prepare_and_run_next_test(test);
  }
  else {
    do_test_finished();
  }
}
