




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
                                  "resource://gre/modules/Deprecated.jsm");

const TOPIC_SHUTDOWN = "places-shutdown";




function TaggingService() {
  
  PlacesUtils.bookmarks.addObserver(this, false);

  
  Services.obs.addObserver(this, TOPIC_SHUTDOWN, false);
}

TaggingService.prototype = {
  






  _createTag: function TS__createTag(aTagName) {
    var newFolderId = PlacesUtils.bookmarks.createFolder(
      PlacesUtils.tagsFolderId, aTagName, PlacesUtils.bookmarks.DEFAULT_INDEX
    );
    
    
    this._tagFolders[newFolderId] = aTagName;

    return newFolderId;
  },

  









  _getItemIdForTaggedURI: function TS__getItemIdForTaggedURI(aURI, aTagName) {
    var tagId = this._getItemIdForTag(aTagName);
    if (tagId == -1)
      return -1;
    
    
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createStatement(
      `SELECT id FROM moz_bookmarks
       WHERE parent = :tag_id
       AND fk = (SELECT id FROM moz_places WHERE url = :page_url)`
    );
    stmt.params.tag_id = tagId;
    stmt.params.page_url = aURI.spec;
    try {
      if (stmt.executeStep()) {
        return stmt.row.id;
      }
    }
    finally {
      stmt.finalize();
    }
    return -1;
  },

  





  _getItemIdForTag: function TS_getItemIdForTag(aTagName) {
    for (var i in this._tagFolders) {
      if (aTagName.toLowerCase() == this._tagFolders[i].toLowerCase())
        return parseInt(i);
    }
    return -1;
  },

  











  _convertInputMixedTagsArray(aTags, trim=false) {
    
    return aTags.filter(tag => tag !== undefined)
                .map(idOrName => {
      let tag = {};
      if (typeof(idOrName) == "number" && this._tagFolders[idOrName]) {
        
        tag.id = idOrName;
        
        
        tag.__defineGetter__("name", () => this._tagFolders[tag.id]);
      }
      else if (typeof(idOrName) == "string" && idOrName.length > 0 &&
               idOrName.length <= Ci.nsITaggingService.MAX_TAG_LENGTH) {
        
        tag.name = trim ? idOrName.trim() : idOrName;
        
        
        tag.__defineGetter__("id", () => this._getItemIdForTag(tag.name));
      }
      else {
        throw Cr.NS_ERROR_INVALID_ARG;
      }
      return tag;
    });
  },

  
  tagURI: function TS_tagURI(aURI, aTags)
  {
    if (!aURI || !aTags || !Array.isArray(aTags)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    let tags = this._convertInputMixedTagsArray(aTags, true);

    let taggingFunction = () => {
      for (let tag of tags) {
        if (tag.id == -1) {
          
          this._createTag(tag.name);
        }

        if (this._getItemIdForTaggedURI(aURI, tag.name) == -1) {
          
          
          PlacesUtils.bookmarks.insertBookmark(
            tag.id, aURI, PlacesUtils.bookmarks.DEFAULT_INDEX, null
          );
        }

        
        
        
        if (PlacesUtils.bookmarks.getItemTitle(tag.id) != tag.name) {
          
          PlacesUtils.bookmarks.setItemTitle(tag.id, tag.name);
        }
      }
    };

    
    if (tags.length < 3) {
      taggingFunction();
    } else {
      PlacesUtils.bookmarks.runInBatchMode(taggingFunction, null);
    }
  },

  





  _removeTagIfEmpty: function TS__removeTagIfEmpty(aTagId) {
    let count = 0;
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createStatement(
      `SELECT count(*) AS count FROM moz_bookmarks
       WHERE parent = :tag_id`
    );
    stmt.params.tag_id = aTagId;
    try {
      if (stmt.executeStep()) {
        count = stmt.row.count;
      }
    }
    finally {
      stmt.finalize();
    }

    if (count == 0) {
      PlacesUtils.bookmarks.removeItem(aTagId);
    }
  },

  
  untagURI: function TS_untagURI(aURI, aTags)
  {
    if (!aURI || (aTags && !Array.isArray(aTags))) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    if (!aTags) {
      
      
      aTags = this.getTagsForURI(aURI);
    }

    
    let tags = this._convertInputMixedTagsArray(aTags);

    let isAnyTagNotTrimmed = tags.some(tag => /^\s|\s$/.test(tag.name));
    if (isAnyTagNotTrimmed) {
      Deprecated.warning("At least one tag passed to untagURI was not trimmed",
                         "https://bugzilla.mozilla.org/show_bug.cgi?id=967196");
    }

    let untaggingFunction = () => {
      for (let tag of tags) {
        if (tag.id != -1) {
          
          let itemId = this._getItemIdForTaggedURI(aURI, tag.name);
          if (itemId != -1) {
            
            PlacesUtils.bookmarks.removeItem(itemId);
          }
        }
      }
    };

    
    if (tags.length < 3) {
      untaggingFunction();
    } else {
      PlacesUtils.bookmarks.runInBatchMode(untaggingFunction, null);
    }
  },

  
  getURIsForTag: function TS_getURIsForTag(aTagName) {
    if (!aTagName || aTagName.length == 0)
      throw Cr.NS_ERROR_INVALID_ARG;

    if (/^\s|\s$/.test(aTagName)) {
      Deprecated.warning("Tag passed to getURIsForTag was not trimmed",
                         "https://bugzilla.mozilla.org/show_bug.cgi?id=967196");
    }

    let uris = [];
    let tagId = this._getItemIdForTag(aTagName);
    if (tagId == -1)
      return uris;

    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createStatement(
      `SELECT h.url FROM moz_places h
       JOIN moz_bookmarks b ON b.fk = h.id
       WHERE b.parent = :tag_id`
    );
    stmt.params.tag_id = tagId;
    try {
      while (stmt.executeStep()) {
        try {
          uris.push(Services.io.newURI(stmt.row.url, null, null));
        } catch (ex) {}
      }
    }
    finally {
      stmt.finalize();
    }

    return uris;
  },

  
  getTagsForURI: function TS_getTagsForURI(aURI, aCount) {
    if (!aURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    var tags = [];
    var bookmarkIds = PlacesUtils.bookmarks.getBookmarkIdsForURI(aURI);
    for (var i=0; i < bookmarkIds.length; i++) {
      var folderId = PlacesUtils.bookmarks.getFolderIdForItem(bookmarkIds[i]);
      if (this._tagFolders[folderId])
        tags.push(this._tagFolders[folderId]);
    }

    
    tags.sort(function(a, b) {
        return a.toLowerCase().localeCompare(b.toLowerCase());
      });
    if (aCount)
      aCount.value = tags.length;
    return tags;
  },

  __tagFolders: null, 
  get _tagFolders() {
    if (!this.__tagFolders) {
      this.__tagFolders = [];

      let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                  .DBConnection;
      let stmt = db.createStatement(
        "SELECT id, title FROM moz_bookmarks WHERE parent = :tags_root "
      );
      stmt.params.tags_root = PlacesUtils.tagsFolderId;
      try {
        while (stmt.executeStep()) {
          this.__tagFolders[stmt.row.id] = stmt.row.title;
        }
      }
      finally {
        stmt.finalize();
      }
    }

    return this.__tagFolders;
  },

  
  get allTags() {
    var allTags = [];
    for (var i in this._tagFolders)
      allTags.push(this._tagFolders[i]);
    
    allTags.sort(function(a, b) {
        return a.toLowerCase().localeCompare(b.toLowerCase());
      });
    return allTags;
  },

  
  get hasTags() {
    return this._tagFolders.length > 0;
  },

  
  observe: function TS_observe(aSubject, aTopic, aData) {
    if (aTopic == TOPIC_SHUTDOWN) {
      PlacesUtils.bookmarks.removeObserver(this);
      Services.obs.removeObserver(this, TOPIC_SHUTDOWN);
    }
  },

  










  _getTaggedItemIdsIfUnbookmarkedURI:
  function TS__getTaggedItemIdsIfUnbookmarkedURI(aURI) {
    var itemIds = [];
    var isBookmarked = false;

    
    
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection;
    let stmt = db.createStatement(
      `SELECT id, parent
       FROM moz_bookmarks
       WHERE fk = (SELECT id FROM moz_places WHERE url = :page_url)`
    );
    stmt.params.page_url = aURI.spec;
    try {
      while (stmt.executeStep() && !isBookmarked) {
        if (this._tagFolders[stmt.row.parent]) {
          
          itemIds.push(stmt.row.id);
        }
        else {
          
          isBookmarked = true;
        }
      }
    }
    finally {
      stmt.finalize();
    }

    return isBookmarked ? [] : itemIds;
  },

  
  onItemAdded: function TS_onItemAdded(aItemId, aFolderId, aIndex, aItemType,
                                       aURI, aTitle) {
    
    if (aFolderId != PlacesUtils.tagsFolderId ||
        aItemType != PlacesUtils.bookmarks.TYPE_FOLDER)
      return;

    this._tagFolders[aItemId] = aTitle;
  },

  onItemRemoved: function TS_onItemRemoved(aItemId, aFolderId, aIndex,
                                           aItemType, aURI) {
    
    if (aFolderId == PlacesUtils.tagsFolderId && this._tagFolders[aItemId]) {
      delete this._tagFolders[aItemId];
    }
    
    else if (aURI && !this._tagFolders[aFolderId]) {
      
      
      
      let itemIds = this._getTaggedItemIdsIfUnbookmarkedURI(aURI);
      for (let i = 0; i < itemIds.length; i++) {
        try {
          PlacesUtils.bookmarks.removeItem(itemIds[i]);
        } catch (ex) {}
      }
    }
    
    else if (aURI && this._tagFolders[aFolderId]) {
      this._removeTagIfEmpty(aFolderId);
    }
  },

  onItemChanged: function TS_onItemChanged(aItemId, aProperty,
                                           aIsAnnotationProperty, aNewValue,
                                           aLastModified, aItemType) {
    if (aProperty == "title" && this._tagFolders[aItemId])
      this._tagFolders[aItemId] = aNewValue;
  },

  onItemMoved: function TS_onItemMoved(aItemId, aOldParent, aOldIndex,
                                      aNewParent, aNewIndex, aItemType) {
    if (this._tagFolders[aItemId] && PlacesUtils.tagsFolderId == aOldParent &&
        PlacesUtils.tagsFolderId != aNewParent)
      delete this._tagFolders[aItemId];
  },

  onItemVisited: function () {},
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},

  
  

  classID: Components.ID("{bbc23860-2553-479d-8b78-94d9038334f7}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(TaggingService),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITaggingService
  , Ci.nsINavBookmarkObserver
  , Ci.nsIObserver
  ])
};


function TagAutoCompleteResult(searchString, searchResult,
                               defaultIndex, errorDescription,
                               results, comments) {
  this._searchString = searchString;
  this._searchResult = searchResult;
  this._defaultIndex = defaultIndex;
  this._errorDescription = errorDescription;
  this._results = results;
  this._comments = comments;
}

TagAutoCompleteResult.prototype = {
  
  


  get searchString() {
    return this._searchString;
  },

  






  get searchResult() {
    return this._searchResult;
  },

  


  get defaultIndex() {
    return this._defaultIndex;
  },

  


  get errorDescription() {
    return this._errorDescription;
  },

  


  get matchCount() {
    return this._results.length;
  },

  get typeAheadResult() false,

  


  getValueAt: function PTACR_getValueAt(index) {
    return this._results[index];
  },

  getLabelAt: function PTACR_getLabelAt(index) {
    return this.getValueAt(index);
  },

  


  getCommentAt: function PTACR_getCommentAt(index) {
    return this._comments[index];
  },

  


  getStyleAt: function PTACR_getStyleAt(index) {
    if (!this._comments[index])
      return null;  

    if (index == 0)
      return "suggestfirst";  

    return "suggesthint";   
  },

  


  getImageAt: function PTACR_getImageAt(index) {
    return null;
  },

  


  getFinalCompleteValueAt: function PTACR_getFinalCompleteValueAt(index) {
    return this.getValueAt(index);
  },

  




  removeValueAt: function PTACR_removeValueAt(index, removeFromDb) {
    this._results.splice(index, 1);
    this._comments.splice(index, 1);
  },

  
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteResult
  ])
};


function TagAutoCompleteSearch() {
  XPCOMUtils.defineLazyServiceGetter(this, "tagging",
                                     "@mozilla.org/browser/tagging-service;1",
                                     "nsITaggingService");
}

TagAutoCompleteSearch.prototype = {
  _stopped : false, 

  








  startSearch: function PTACS_startSearch(searchString, searchParam, result, listener) {
    var searchResults = this.tagging.allTags;
    var results = [];
    var comments = [];
    this._stopped = false;

    
    var index = Math.max(searchString.lastIndexOf(","), 
      searchString.lastIndexOf(";"));
    var before = ''; 
    if (index != -1) {  
      before = searchString.slice(0, index+1);
      searchString = searchString.slice(index+1);
      
      var m = searchString.match(/\s+/);
      if (m) {
         before += m[0];
         searchString = searchString.slice(m[0].length);
      }
    }

    if (!searchString.length) {
      var newResult = new TagAutoCompleteResult(searchString,
        Ci.nsIAutoCompleteResult.RESULT_NOMATCH, 0, "", results, comments);
      listener.onSearchResult(self, newResult);
      return;
    }
    
    var self = this;
    
    function doSearch() {
      var i = 0;
      while (i < searchResults.length) {
        if (self._stopped)
          yield false;
        
        if (searchResults[i].toLowerCase()
                            .indexOf(searchString.toLowerCase()) == 0 &&
            comments.indexOf(searchResults[i]) == -1) {
          results.push(before + searchResults[i]);
          comments.push(searchResults[i]);
        }
    
        ++i;

        








        








      }

      let searchResult = results.length > 0 ?
                           Ci.nsIAutoCompleteResult.RESULT_SUCCESS :
                           Ci.nsIAutoCompleteResult.RESULT_NOMATCH;
      var newResult = new TagAutoCompleteResult(searchString, searchResult, 0,
                                                "", results, comments);
      listener.onSearchResult(self, newResult);
      yield false;
    }
    
    
    var gen = doSearch();
    while (gen.next());
    gen.close();
  },

  


  stopSearch: function PTACS_stopSearch() {
    this._stopped = true;
  },

  
  

  classID: Components.ID("{1dcc23b0-d4cb-11dc-9ad6-479d56d89593}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(TagAutoCompleteSearch),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteSearch
  ])
};

let component = [TaggingService, TagAutoCompleteSearch];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
