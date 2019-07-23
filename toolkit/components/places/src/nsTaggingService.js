








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const NH_CONTRACTID = "@mozilla.org/browser/nav-history-service;1";
const BMS_CONTRACTID = "@mozilla.org/browser/nav-bookmarks-service;1";
const IO_CONTRACTID = "@mozilla.org/network/io-service;1";
const ANNO_CONTRACTID = "@mozilla.org/browser/annotation-service;1";
const FAV_CONTRACTID = "@mozilla.org/browser/favicon-service;1";
const OBSS_CONTRACTID = "@mozilla.org/observer-service;1";

var gIoService = Cc[IO_CONTRACTID].getService(Ci.nsIIOService);




function TaggingService() {
  this._bms = Cc[BMS_CONTRACTID].getService(Ci.nsINavBookmarksService);
  this._bms.addObserver(this, false);

  this._obss = Cc[OBSS_CONTRACTID].getService(Ci.nsIObserverService);
  this._obss.addObserver(this, "xpcom-shutdown", false);
}

TaggingService.prototype = {
  get _history() {
    if (!this.__history)
      this.__history = Cc[NH_CONTRACTID].getService(Ci.nsINavHistoryService);
    return this.__history;
  },

  get _annos() {
    if (!this.__annos)
      this.__annos =  Cc[ANNO_CONTRACTID].getService(Ci.nsIAnnotationService);
    return this.__annos;
  },

  
  classDescription: "Places Tagging Service",
  contractID: "@mozilla.org/browser/tagging-service;1",
  classID: Components.ID("{bbc23860-2553-479d-8b78-94d9038334f7}"),

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITaggingService,
                                         Ci.nsINavBookmarkObserver,
                                         Ci.nsIObserver]),

  


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

    var options = this._history.getNewQueryOptions();
    var query = this._history.getNewQuery();
    query.setFolders([tagId], 1);
    var result = this._history.executeQuery(query, options);
    return result;
  },

  






  _createTag: function TS__createTag(aTagName) {
    var newFolderId = this._bms.createFolder(this._bms.tagsFolder, aTagName,
                                             this._bms.DEFAULT_INDEX);
    
    
    this._tagFolders[newFolderId] = aTagName;

    return newFolderId;
  },

  









  _getItemIdForTaggedURI: function TS__getItemIdForTaggedURI(aURI, aTagName) {
    var tagId = this._getItemIdForTag(aTagName);
    if (tagId == -1)
      return -1;
    var bookmarkIds = this._bms.getBookmarkIdsForURI(aURI);
    for (var i=0; i < bookmarkIds.length; i++) {
      var parent = this._bms.getFolderIdForItem(bookmarkIds[i]);
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

  
  tagURI: function TS_tagURI(aURI, aTags) {
    if (!aURI || !aTags)
      throw Cr.NS_ERROR_INVALID_ARG;

    this._bms.runInBatchMode({
      _self: this,
      runBatched: function(aUserData) {
        for (var i = 0; i < aTags.length; i++) {
          var tag = aTags[i];
          var tagId = null;
          if (typeof(tag) == "number") {
            
            if (this._self._tagFolders[tag]) {
              tagId = tag;
              tag = this._self._tagFolders[tagId];
            }
            else
              throw Cr.NS_ERROR_INVALID_ARG;
          }
          else {
            tagId = this._self._getItemIdForTag(tag);
            if (tagId == -1)
              tagId = this._self._createTag(tag);
          }

          var itemId = this._self._getItemIdForTaggedURI(aURI, tag);
          if (itemId == -1)
            this._self._bms.insertBookmark(tagId, aURI,
                                           this._self._bms.DEFAULT_INDEX, null);

          
          
          var currentTagTitle = this._self._bms.getItemTitle(tagId);
          if (currentTagTitle != tag) {
            this._self._bms.setItemTitle(tagId, tag);
            this._self._tagFolders[tagId] = tag;
          }
        }
      }
    }, null);
  },

  





  _removeTagIfEmpty: function TS__removeTagIfEmpty(aTagId) {
    var result = this._getTagResult(aTagId);
    if (!result)
      return;
    var node = result.root;
    node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    node.containerOpen = true;
    var cc = node.childCount;
    node.containerOpen = false;
    if (cc == 0)
      this._bms.removeItem(node.itemId);
  },

  
  untagURI: function TS_untagURI(aURI, aTags) {
    if (!aURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    if (!aTags) {
      
      
      aTags = this.getTagsForURI(aURI);
    }

    this._bms.runInBatchMode({
      _self: this,
      runBatched: function(aUserData) {
        for (var i = 0; i < aTags.length; i++) {
          var tag = aTags[i];
          var tagId = null;
          if (typeof(tag) == "number") {
            
            if (this._self._tagFolders[tag]) {
              tagId = tag;
              tag = this._self._tagFolders[tagId];
            }
            else
              throw Cr.NS_ERROR_INVALID_ARG;
          }
          else
            tagId = this._self._getItemIdForTag(tag);

          if (tagId != -1) {
            var itemId = this._self._getItemIdForTaggedURI(aURI, tag);
            if (itemId != -1) {
              this._self._bms.removeItem(itemId);
              this._self._removeTagIfEmpty(tagId);
            }
          }
        }
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
          uris.push(gIoService.newURI(tagNode.getChild(i).uri, null, null));
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
    var bookmarkIds = this._bms.getBookmarkIdsForURI(aURI);
    for (var i=0; i < bookmarkIds.length; i++) {
      var folderId = this._bms.getFolderIdForItem(bookmarkIds[i]);
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
      var options = this._history.getNewQueryOptions();
      var query = this._history.getNewQuery();
      query.setFolders([this._bms.tagsFolder], 1);
      var tagsResult = this._history.executeQuery(query, options);
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
    if (aTopic == "xpcom-shutdown") {
      this._bms.removeObserver(this);
      this._obss.removeObserver(this, "xpcom-shutdown");
    }
  },

  










  _getTagsIfUnbookmarkedURI: function TS__getTagsIfUnbookmarkedURI(aURI) {
    var tagIds = [];
    var isBookmarked = false;
    var itemIds = this._bms.getBookmarkIdsForURI(aURI);

    for (let i = 0; !isBookmarked && i < itemIds.length; i++) {
      var parentId = this._bms.getFolderIdForItem(itemIds[i]);
      if (this._tagFolders[parentId])
        tagIds.push(parentId);
      else
        isBookmarked = true;
    }

    return !isBookmarked && tagIds.length > 0 ? tagIds : null;
  },

  
  _inBatch: false,

  
  _itemsInRemoval: {},

  
  onBeginUpdateBatch: function() {
    this._inBatch = true;
  },
  onEndUpdateBatch: function() {
    this._inBatch = false;
  },

  onItemAdded: function(aItemId, aFolderId, aIndex, aItemType) {
    
    if (aFolderId != this._bms.tagsFolder ||
        aItemType != this._bms.TYPE_FOLDER)
      return;

    this._tagFolders[aItemId] = this._bms.getItemTitle(aItemId);
  },

  onBeforeItemRemoved: function(aItemId, aItemType) {
    if (aItemType == this._bms.TYPE_BOOKMARK)
      this._itemsInRemoval[aItemId] = this._bms.getBookmarkURI(aItemId);
  },

  onItemRemoved: function(aItemId, aFolderId, aIndex, aItemType) {
    var itemURI = this._itemsInRemoval[aItemId];
    delete this._itemsInRemoval[aItemId];

    
    if (aFolderId == this._bms.tagsFolder && this._tagFolders[aItemId])
      delete this._tagFolders[aItemId];

    
    else if (itemURI && !this._tagFolders[aFolderId]) {

      
      
      
      var tagIds = this._getTagsIfUnbookmarkedURI(itemURI);
      if (tagIds)
        this.untagURI(itemURI, tagIds);
    }
  },

  onItemChanged: function(aItemId, aProperty, aIsAnnotationProperty, aNewValue,
                          aLastModified, aItemType) {
    if (aProperty == "title" && this._tagFolders[aItemId])
      this._tagFolders[aItemId] = this._bms.getItemTitle(aItemId);
  },

  onItemVisited: function(aItemId, aVisitID, time) {},

  onItemMoved: function(aItemId, aOldParent, aOldIndex, aNewParent, aNewIndex,
                        aItemType) {
    if (this._tagFolders[aItemId] && this._bms.tagFolder == aOldParent &&
        this._bms.tagFolder != aNewParent)
      delete this._tagFolders[aItemId];
  }
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

  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsIAutoCompleteResult) && !aIID.equals(Ci.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


function TagAutoCompleteSearch() {
}

TagAutoCompleteSearch.prototype = {
  _stopped : false, 

  get tagging() {
    let svc = Cc["@mozilla.org/browser/tagging-service;1"].
              getService(Ci.nsITaggingService);
    this.__defineGetter__("tagging", function() svc);
    return this.tagging;
  },

  








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

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteSearch,
                                         Ci.nsITimerCallback]), 

  classDescription: "Places Tag AutoComplete",
  contractID: "@mozilla.org/autocomplete/search;1?name=places-tag-autocomplete",
  classID: Components.ID("{1dcc23b0-d4cb-11dc-9ad6-479d56d89593}")
};

var component = [TaggingService, TagAutoCompleteSearch];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(component);
}
