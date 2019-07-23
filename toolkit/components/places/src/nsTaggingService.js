





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const NH_CONTRACTID = "@mozilla.org/browser/nav-history-service;1";
const BMS_CONTRACTID = "@mozilla.org/browser/nav-bookmarks-service;1";
const IO_CONTRACTID = "@mozilla.org/network/io-service;1";
const ANNO_CONTRACTID = "@mozilla.org/browser/annotation-service;1";
const FAV_CONTRACTID = "@mozilla.org/browser/favicon-service;1";
const OBSS_CONTRACTID = "@mozilla.org/observer-service;1";

var gIoService = Cc[IO_CONTRACTID].getService(Ci.nsIIOService);




function TaggingService() {
}

TaggingService.prototype = {
  get _bms() {
    if (!this.__bms)
      this.__bms = Cc[BMS_CONTRACTID].getService(Ci.nsINavBookmarksService);
    return this.__bms;
  },

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

  get _tagsResult() {
    if (!this.__tagsResult) {
      var options = this._history.getNewQueryOptions();
      var query = this._history.getNewQuery();
      query.setFolders([this._bms.tagsFolder], 1);
      this.__tagsResult = this._history.executeQuery(query, options);
      this.__tagsResult.root.containerOpen = true;

      
      var observerSvc = Cc[OBSS_CONTRACTID].getService(Ci.nsIObserverService);
      observerSvc.addObserver(this, "xpcom-shutdown", false);
    }
    return this.__tagsResult;
  },

  
  classDescription: "Places Tagging Service",
  contractID: "@mozilla.org/browser/tagging-service;1",
  classID: Components.ID("{bbc23860-2553-479d-8b78-94d9038334f7}"),

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITaggingService,
                                         Ci.nsIObserver]),

  


  _getTagNode: function TS__getTagIndex(aTagNameOrId) {
    if (!aTagNameOrId)
      throw Cr.NS_ERROR_INVALID_ARG;

    var nameLower = null;
    if (typeof(aTagNameOrId) == "string")
      nameLower = aTagNameOrId.toLowerCase();

    var root = this._tagsResult.root;
    var cc = root.childCount;
    for (var i=0; i < cc; i++) {
      var child = root.getChild(i);
      if ((nameLower && child.title.toLowerCase() == nameLower) ||
          child.itemId === aTagNameOrId)
        return child;
    }

    return null;
  },

  






  _createTag: function TS__createTag(aName) {
    return this._bms.createFolder(this._bms.tagsFolder, aName,
                                  this._bms.DEFAULT_INDEX);
  },

  











  _isURITaggedInternal: function TS__uriTagged(aURI, aTagId, aItemId) {
    var bookmarkIds = this._bms.getBookmarkIdsForURI(aURI, {});
    for (var i=0; i < bookmarkIds.length; i++) {
      var parent = this._bms.getFolderIdForItem(bookmarkIds[i]);
      if (parent == aTagId) {
        aItemId.value = bookmarkIds[i];
        return true;
      }
    }
    return false;
  },

  
  tagURI: function TS_tagURI(aURI, aTags) {
    if (!aURI || !aTags)
      throw Cr.NS_ERROR_INVALID_ARG;

    for (var i=0; i < aTags.length; i++) {
      var tagNode = this._getTagNode(aTags[i]);
      if (!tagNode) {
        if (typeof(aTags[i]) == "number")
          throw Cr.NS_ERROR_INVALID_ARG;

        var tagId = this._createTag(aTags[i]);
        this._bms.insertBookmark(tagId, aURI, this._bms.DEFAULT_INDEX, null);
      }
      else {
        var tagId = tagNode.itemId;
        if (!this._isURITaggedInternal(aURI, tagNode.itemId, {}))
          this._bms.insertBookmark(tagId, aURI, this._bms.DEFAULT_INDEX, null);

        
        
        
        if (typeof(aTags[i]) == "string" && tagNode.title != aTags[i])
          this._bms.setItemTitle(tagNode.itemId, aTags[i]);
      }
    }
  },

  





  _removeTagIfEmpty: function TS__removeTagIfEmpty(aTagId) {
    var node = this._getTagNode(aTagId).QueryInterface(Ci.nsINavHistoryContainerResultNode);
    var wasOpen = node.containerOpen;
    if (!wasOpen)
      node.containerOpen = true;
    var cc = node.childCount;
    if (wasOpen)
      node.containerOpen = false;
    if (cc == 0) {
      this._bms.removeFolder(node.itemId);
    }
  },

  
  untagURI: function TS_untagURI(aURI, aTags) {
    if (!aURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    if (!aTags) {
      
      
      aTags = this.getTagsForURI(aURI, { });
    }

    for (var i=0; i < aTags.length; i++) {
      var tagNode = this._getTagNode(aTags[i]);
      if (tagNode) {
        var itemId = { };
        if (this._isURITaggedInternal(aURI, tagNode.itemId, itemId)) {
          this._bms.removeItem(itemId.value);
          this._removeTagIfEmpty(tagNode.itemId);
        }
      }
      else if (typeof(aTags[i]) == "number")
        throw Cr.NS_ERROR_INVALID_ARG;
    }
  },

  
  getURIsForTag: function TS_getURIsForTag(aTag) {
    if (aTag.length == 0)
      throw Cr.NS_ERROR_INVALID_ARG;

    var uris = [];
    var tagNode = this._getTagNode(aTag);
    if (tagNode) {
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
    var bookmarkIds = this._bms.getBookmarkIdsForURI(aURI, {});
    var root = this._tagsResult.root;
    var cc = root.childCount;
    for (var i=0; i < bookmarkIds.length; i++) {
      var parent = this._bms.getFolderIdForItem(bookmarkIds[i]);
      for (var j=0; j < cc; j++) {
        var child = root.getChild(j);
        if (child.itemId == parent)
          tags.push(child.title);
      }
    }

    
    tags.sort();
    aCount.value = tags.length;
    return tags;
  },

  
  get allTags() {
    var tags = [];
    var root = this._tagsResult.root;
    var cc = root.childCount;
    for (var j=0; j < cc; j++) {
      var child = root.getChild(j);
      tags.push(child.title);
    }

    
    tags.sort();
    return tags;
  },

  
  observe: function TS_observe(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      this.__tagsResult.root.containerOpen = false;
      this.__tagsResult = null;
      var observerSvc = Cc[OBSS_CONTRACTID].getService(Ci.nsIObserverService);
      observerSvc.removeObserver(this, "xpcom-shutdown");
    }
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([TaggingService]);
}
