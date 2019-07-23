





































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

const TAG_CONTAINER_ICON_URI = "chrome://mozapps/skin/places/tagContainerIcon.png"

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
      query.setFolders([this._bms.tagRoot], 1);
      this.__tagsResult = this._history.executeQuery(query, options);
      this.__tagsResult.root.containerOpen = true;

      
      var observerSvc = Cc[OBSS_CONTRACTID].getService(Ci.nsIObserverService);
      observerSvc.addObserver(this, "xpcom-shutdown", false);
    }
    return this.__tagsResult;
  },

  
  classDescription: "Places Tagging Service",
  contractID: "@mozilla.org/browser/tagging-service;1",
  classID: Components.ID("{6a059068-1630-11dc-8314-0800200c9a66}"),

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITaggingService,
                                         Ci.nsIObserver]),

  


  _getTagNode: function TS__getTagIndex(aName) {
    var root = this._tagsResult.root;
    var cc = root.childCount;
    for (var i=0; i < cc; i++) {
      var child = root.getChild(i);
      if (child.title == aName)
        return child;
    }

    return null;
  },

  get _tagContainerIcon() {
    if (!this.__tagContainerIcon) {
      this.__tagContainerIcon =
        gIoService.newURI(TAG_CONTAINER_ICON_URI, null, null);
    }

    return this.__tagContainerIcon;
  },

  






  _createTag: function TS__createTag(aName) {
    var id = this._bms.createFolder(this._bms.tagRoot, aName,
                                    this._bms.DEFAULT_INDEX);

    
    var faviconService = Cc[FAV_CONTRACTID].getService(Ci.nsIFaviconService);
    var uri = this._bms.getFolderURI(id);
    faviconService.setFaviconUrlForPage(uri, this._tagContainerIcon);
  
    return id;
  },

  











  _isURITaggedInternal: function TS__uriTagged(aURI, aTagId, aItemId) {
    var options = this._history.getNewQueryOptions();
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    var query = this._history.getNewQuery();
    query.setFolders([aTagId], 1);
    query.uri = aURI;
    var result = this._history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    if (rootNode.childCount != 0) {
      aItemId.value = rootNode.getChild(0).itemId;
      return true;
    }
    return false;
  },

  
  tagURI: function TS_tagURI(aURI, aTags) {
    if (!aURI || !aTags)
      throw Cr.NS_ERROR_INVALID_ARG;

    for (var i=0; i < aTags.length; i++) {
      if (aTags[i].length == 0)
        throw Cr.NS_ERROR_INVALID_ARG;

      var tagNode = this._getTagNode(aTags[i]);
      if (!tagNode) {
        var tagId = this._createTag(aTags[i]);
        this._bms.insertBookmark(tagId, aURI, this._bms.DEFAULT_INDEX, "");
      }
      else {
        var tagId = tagNode.itemId;
        if (!this._isURITaggedInternal(aURI, tagNode.itemId, {}))
          this._bms.insertBookmark(tagId, aURI, this._bms.DEFAULT_INDEX, "");
      }
    }
  },

  





  _removeTagIfEmpty: function TS__removeTagIfEmpty(aTagId) {
    var options = this._history.getNewQueryOptions();
    var query = this._history.getNewQuery();
    query.setFolders([aTagId], 1);
    var result = this._history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    if (rootNode.childCount == 0)
      this._bms.removeFolder(aTagId);
  },

  
  untagURI: function TS_untagURI(aURI, aTags) {
    if (!aURI)
      throw Cr.NS_ERROR_INVALID_ARG;

    if (!aTags) {
      
      
      aTags = this.getTagsForURI(aURI);
    }

    for (var i=0; i < aTags.length; i++) {
      if (aTags[i].length == 0)
        throw Cr.NS_ERROR_INVALID_ARG;

      var tagNode = this._getTagNode(aTags[i]);
      if (tagNode) {
        var itemId = { };
        if (this._isURITaggedInternal(aURI, tagNode.itemId, itemId)) {
          this._bms.removeItem(itemId.value);
          this._removeTagIfEmpty(tagNode.itemId);
        }
      }
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
      for (var i=0; i < cc; i++)
        uris.push(gIoService.newURI(tagNode.getChild(i).uri, null, null));
      tagNode.containerOpen = false;
    }
    return uris;
  },

  
  getTagsForURI: function TS_getTagsForURI(aURI) {
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
