








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PlacesUtils.jsm");

const TOPIC_SHUTDOWN = "places-shutdown";




function TaggingService() {
  
  PlacesUtils.bookmarks.addObserver(this, false);

  
  Services.obs.addObserver(this, TOPIC_SHUTDOWN, false);
}

TaggingService.prototype = {
  


  _getTagResult: function TS__getTagResult(aTagNameOrId) {
    if (!aTagNameOrId)
      throw Cr.NS_ERROR_INVALID_ARG;

    var tagId = null;
    if (typeof(aTagNameOrId) == "string")
      tagId = this._getItemIdForTag(aTagNameOrId);
    else
      tagId = aTagNameOrId;

    if (tagId == -1)
      return null;

    var options = PlacesUtils.history.getNewQueryOptions();
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([tagId], 1);
    var result = PlacesUtils.history.executeQuery(query, options);
    return result;
  },

  






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
    var bookmarkIds = PlacesUtils.bookmarks.getBookmarkIdsForURI(aURI);
    for (var i=0; i < bookmarkIds.length; i++) {
      var parent = PlacesUtils.bookmarks.getFolderIdForItem(bookmarkIds[i]);
      if (parent == tagId)
        return bookmarkIds[i];
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

  









  _convertInputMixedTagsArray: function TS__convertInputMixedTagsArray(aTags)
  {
    return aTags.map(function (val)
    {
      let tag = { _self: this };
      if (typeof(val) == "number" && this._tagFolders[val]) {
        
        tag.id = val;
        
        
        tag.__defineGetter__("name", function () this._self._tagFolders[this.id]);
      }
      else if (typeof(val) == "string" && val.length > 0) {
        
        tag.name = val;
        
        
        tag.__defineGetter__("id", function () this._self._getItemIdForTag(this.name));
      }
      else {
        throw Cr.NS_ERROR_INVALID_ARG;
      }
      return tag;
    }, this);
  },

  
  tagURI: function TS_tagURI(aURI, aTags)
  {
    if (!aURI || !aTags || !Array.isArray(aTags)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    
    let tags = this._convertInputMixedTagsArray(aTags);

    let taggingService = this;
    PlacesUtils.bookmarks.runInBatchMode({
      runBatched: function (aUserData)
      {
        tags.forEach(function (tag)
        {
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
        }, taggingService);
      }
    }, null);
  },

  





  _removeTagIfEmpty: function TS__removeTagIfEmpty(aTagId) {
    var result = this._getTagResult(aTagId);
    if (!result)
      return;
    var node = PlacesUtils.asContainer(result.root);
    node.containerOpen = true;
    var cc = node.childCount;
    node.containerOpen = false;
    if (cc == 0) {
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

    let taggingService = this;
    PlacesUtils.bookmarks.runInBatchMode({
      runBatched: function (aUserData)
      {
        tags.forEach(function (tag)
        {
          if (tag.id != -1) {
            
            let itemId = this._getItemIdForTaggedURI(aURI, tag.name);
            if (itemId != -1) {
              
              PlacesUtils.bookmarks.removeItem(itemId);
            }
          }
        }, taggingService);
      }
    }, null);
  },

  
  getURIsForTag: function TS_getURIsForTag(aTag) {
    if (!aTag || aTag.length == 0)
      throw Cr.NS_ERROR_INVALID_ARG;

    var uris = [];
    var tagResult = this._getTagResult(aTag);
    if (tagResult) {
      var tagNode = tagResult.root;
      tagNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      tagNode.containerOpen = true;
      var cc = tagNode.childCount;
      for (var i = 0; i < cc; i++) {
        try {
          uris.push(Services.io.newURI(tagNode.getChild(i).uri, null, null));
        } catch (ex) {
          
          
        }
      }
      tagNode.containerOpen = false;
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
      var options = PlacesUtils.history.getNewQueryOptions();
      var query = PlacesUtils.history.getNewQuery();
      query.setFolders([PlacesUtils.tagsFolderId], 1);
      var tagsResult = PlacesUtils.history.executeQuery(query, options);
      var root = tagsResult.root;
      root.containerOpen = true;
      var cc = root.childCount;
      for (var i=0; i < cc; i++) {
        var child = root.getChild(i);
        this.__tagFolders[child.itemId] = child.title;
      }
      root.containerOpen = false;
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

  
  observe: function TS_observe(aSubject, aTopic, aData) {
    if (aTopic == TOPIC_SHUTDOWN) {
      PlacesUtils.bookmarks.removeObserver(this);
      Services.obs.removeObserver(this, TOPIC_SHUTDOWN);
    }
  },

  










  _getTagsIfUnbookmarkedURI: function TS__getTagsIfUnbookmarkedURI(aURI) {
    var tagIds = [];
    var isBookmarked = false;
    var itemIds = PlacesUtils.bookmarks.getBookmarkIdsForURI(aURI);

    for (let i = 0; !isBookmarked && i < itemIds.length; i++) {
      var parentId = PlacesUtils.bookmarks.getFolderIdForItem(itemIds[i]);
      if (this._tagFolders[parentId])
        tagIds.push(parentId);
      else
        isBookmarked = true;
    }

    return !isBookmarked && tagIds.length > 0 ? tagIds : null;
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
    
    if (aFolderId == PlacesUtils.tagsFolderId && this._tagFolders[aItemId])
      delete this._tagFolders[aItemId];

    
    else if (aURI && !this._tagFolders[aFolderId]) {

      
      
      
      var tagIds = this._getTagsIfUnbookmarkedURI(aURI);
      if (tagIds)
        this.untagURI(aURI, tagIds);
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
  onBeforeItemRemoved: function () {},
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},

  
  classID: Components.ID("{bbc23860-2553-479d-8b78-94d9038334f7}"),
  
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

      var newResult = new TagAutoCompleteResult(searchString,
        Ci.nsIAutoCompleteResult.RESULT_SUCCESS, 0, "", results, comments);
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

  
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteSearch
  ]),

  classID: Components.ID("{1dcc23b0-d4cb-11dc-9ad6-479d56d89593}")
};

let component = [TaggingService, TagAutoCompleteSearch];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
