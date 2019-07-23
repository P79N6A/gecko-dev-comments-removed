








































function LOG(str) {
  dump("*** " + str + "\n");
}

var EXPORTED_SYMBOLS = ["PlacesUtils"];

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

const POST_DATA_ANNO = "bookmarkProperties/POSTData";
const READ_ONLY_ANNO = "placesInternal/READ_ONLY";
const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";

#ifdef XP_MACOSX


const NEWLINE= "\n";
#else

const NEWLINE = "\r\n";
#endif

function QI_node(aNode, aIID) {
  var result = null;
  try {
    result = aNode.QueryInterface(aIID);
  }
  catch (e) {
  }
  return result;
}
function asVisit(aNode)    { return QI_node(aNode, Ci.nsINavHistoryVisitResultNode);    }
function asFullVisit(aNode){ return QI_node(aNode, Ci.nsINavHistoryFullVisitResultNode);}
function asContainer(aNode){ return QI_node(aNode, Ci.nsINavHistoryContainerResultNode);}
function asQuery(aNode)    { return QI_node(aNode, Ci.nsINavHistoryQueryResultNode);    }

var PlacesUtils = {
  
  TYPE_X_MOZ_PLACE_CONTAINER: "text/x-moz-place-container",
  
  TYPE_X_MOZ_PLACE_SEPARATOR: "text/x-moz-place-separator",
  
  TYPE_X_MOZ_PLACE: "text/x-moz-place",
  
  TYPE_X_MOZ_URL: "text/x-moz-url",
  
  TYPE_HTML: "text/html",
  
  TYPE_UNICODE: "text/unicode",

  


  get bookmarks() {
    delete this.bookmarks;
    return this.bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                            getService(Ci.nsINavBookmarksService);
  },

  


  get history() {
    delete this.history;
    return this.history = Cc["@mozilla.org/browser/nav-history-service;1"].
                          getService(Ci.nsINavHistoryService);
  },

  


  get livemarks() {
    delete this.livemarks;
    return this.livemarks = Cc["@mozilla.org/browser/livemark-service;2"].
                            getService(Ci.nsILivemarkService);
  },

  


  get annotations() {
    delete this.annotations;
    return this.annotations = Cc["@mozilla.org/browser/annotation-service;1"].
                              getService(Ci.nsIAnnotationService);
  },

  


  get favicons() {
    delete this.favicons;
    return this.favicons = Cc["@mozilla.org/browser/favicon-service;1"].
                           getService(Ci.nsIFaviconService);
  },

  


  get tagging() {
    delete this.tagging;
    return this.tagging = Cc["@mozilla.org/browser/tagging-service;1"].
                          getService(Ci.nsITaggingService);
  },

  





  _uri: function PU__uri(aSpec) {
    return Cc["@mozilla.org/network/io-service;1"].
           getService(Ci.nsIIOService).
           newURI(aSpec, null, null);
  },

  


  get _bundle() {
    const PLACES_STRING_BUNDLE_URI =
        "chrome://places/locale/places.properties";
    delete this._bundle;
    return this._bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                          getService(Ci.nsIStringBundleService).
                          createBundle(PLACES_STRING_BUNDLE_URI);
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PU_getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  





  nodeIsFolder: function PU_nodeIsFolder(aNode) {
    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER ||
            aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT);
  },

  





  nodeIsBookmark: function PU_nodeIsBookmark(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI &&
           aNode.itemId != -1;
  },

  





  nodeIsSeparator: function PU_nodeIsSeparator(aNode) {

    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR);
  },

  





  nodeIsVisit: function PU_nodeIsVisit(aNode) {
    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_VISIT ||
           type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  





  uriTypes: [Ci.nsINavHistoryResultNode.RESULT_TYPE_URI,
             Ci.nsINavHistoryResultNode.RESULT_TYPE_VISIT,
             Ci.nsINavHistoryResultNode.RESULT_TYPE_FULL_VISIT],
  nodeIsURI: function PU_nodeIsURI(aNode) {
    return this.uriTypes.indexOf(aNode.type) != -1;
  },

  





  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  






  nodeIsReadOnly: function PU_nodeIsReadOnly(aNode) {
    if (this.nodeIsFolder(aNode))
      return this.bookmarks.getFolderReadonly(asQuery(aNode).folderItemId);
    if (this.nodeIsQuery(aNode) &&
        asQuery(aNode).queryOptions.resultType !=
          Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_CONTENTS)
      return aNode.childrenReadOnly;
    return false;
  },

  





  nodeIsHost: function PU_nodeIsHost(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY &&
           aNode.parent &&
           asQuery(aNode.parent).queryOptions.resultType ==
             Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY;
  },

  





  nodeIsDay: function PU_nodeIsDay(aNode) {
    var resultType;
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY &&
           aNode.parent &&
           ((resultType = asQuery(aNode.parent).queryOptions.resultType) ==
               Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
             resultType == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY);
  },

  





  nodeIsTagQuery: function PU_nodeIsTagQuery(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY &&
           asQuery(aNode).queryOptions.resultType ==
             Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_CONTENTS;
  },

  





  containerTypes: [Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_DYNAMIC_CONTAINER],
  nodeIsContainer: function PU_nodeIsContainer(aNode) {
    return this.containerTypes.indexOf(aNode.type) != -1;
  },

  





  nodeIsHistoryContainer: function PU_nodeIsHistoryContainer(aNode) {
    var resultType;
    return this.nodeIsQuery(aNode) &&
           ((resultType = asQuery(aNode).queryOptions.resultType) ==
              Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY ||
            resultType == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
            resultType == Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY ||
            this.nodeIsDay(aNode) ||
            this.nodeIsHost(aNode));
  },

  








  nodeIsDynamicContainer: function PU_nodeIsDynamicContainer(aNode) {
    if (aNode.type == NHRN.RESULT_TYPE_DYNAMIC_CONTAINER)
      return true;
    return false;
  },

 






  nodeIsLivemarkContainer: function PU_nodeIsLivemarkContainer(aNode) {
    
    
    return this.nodeIsFolder(aNode) &&
           this.annotations.itemHasAnnotation(aNode.itemId, LMANNO_FEEDURI);
  },

 





  nodeIsLivemarkItem: function PU_nodeIsLivemarkItem(aNode) {
    return aNode.parent && this.nodeIsLivemarkContainer(aNode.parent);
  },

  





  isReadonlyFolder: function(aNode) {
    return this.nodeIsFolder(aNode) &&
           this.bookmarks.getFolderReadonly(asQuery(aNode).folderItemId);
  },

  



  getConcreteItemId: function PU_getConcreteItemId(aNode) {
    if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT)
      return asQuery(aNode).folderItemId;
    else if (PlacesUtils.nodeIsTagQuery(aNode)) {
      
      
      var queries = aNode.getQueries({});
      var folders = queries[0].getFolders({});
      return folders[0];
    }
    return aNode.itemId;
  },

  






  getIndexOfNode: function PU_getIndexOfNode(aNode) {
    var parent = aNode.parent;
    if (!parent)
      return -1;
    var wasOpen = parent.containerOpen;
    var result, oldViewer;
    if (!wasOpen) {
      result = parent.parentResult;
      oldViewer = result.viewer;
      result.viewer = null;
      parent.containerOpen = true;
    }
    var cc = parent.childCount;
    for (var i = 0; i < cc && parent.getChild(i) != aNode; ++i);
    if (!wasOpen) {
      parent.containerOpen = false;
      result.viewer = oldViewer;
    }
    return i < cc ? i : -1;
  },

  














  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI, aForceCopy) {
    var self = this;

    
    
    
    
    function convertNode(cNode) {
      if (self.nodeIsFolder(cNode) && asQuery(cNode).queryOptions.excludeItems) {
        var concreteId = self.getConcreteItemId(cNode);
        return self.getFolderContents(concreteId, false, true).root;
      }
      return cNode;
    }

    switch (aType) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        var writer = {
          value: "",
          write: function PU_wrapNode__write(aStr, aLen) {
            this.value += aStr;
          }
        };
        self.serializeNodeAsJSONToOutputStream(convertNode(aNode), writer, true, aForceCopy);
        return writer.value;
      case this.TYPE_X_MOZ_URL:
        function gatherDataUrl(bNode) {
          if (self.nodeIsLivemarkContainer(bNode)) {
            var siteURI = self.livemarks.getSiteURI(bNode.itemId).spec;
            return siteURI + NEWLINE + bNode.title;
          }
          if (self.nodeIsURI(bNode))
            return (aOverrideURI || bNode.uri) + NEWLINE + bNode.title;
          
          return "";
        }
        return gatherDataUrl(convertNode(aNode));

      case this.TYPE_HTML:
        function gatherDataHtml(bNode) {
          function htmlEscape(s) {
            s = s.replace(/&/g, "&amp;");
            s = s.replace(/>/g, "&gt;");
            s = s.replace(/</g, "&lt;");
            s = s.replace(/"/g, "&quot;");
            s = s.replace(/'/g, "&apos;");
            return s;
          }
          
          var escapedTitle = bNode.title ? htmlEscape(bNode.title) : "";
          if (self.nodeIsLivemarkContainer(bNode)) {
            var siteURI = self.livemarks.getSiteURI(bNode.itemId).spec;
            return "<A HREF=\"" + siteURI + "\">" + escapedTitle + "</A>" + NEWLINE;
          }
          if (self.nodeIsContainer(bNode)) {
            asContainer(bNode);
            var wasOpen = bNode.containerOpen;
            if (!wasOpen)
              bNode.containerOpen = true;

            var childString = "<DL><DT>" + escapedTitle + "</DT>" + NEWLINE;
            var cc = bNode.childCount;
            for (var i = 0; i < cc; ++i)
              childString += "<DD>"
                             + NEWLINE
                             + gatherDataHtml(bNode.getChild(i))
                             + "</DD>"
                             + NEWLINE;
            bNode.containerOpen = wasOpen;
            return childString + "</DL>" + NEWLINE;
          }
          if (self.nodeIsURI(bNode))
            return "<A HREF=\"" + bNode.uri + "\">" + escapedTitle + "</A>" + NEWLINE;
          if (self.nodeIsSeparator(bNode))
            return "<HR>" + NEWLINE;
          return "";
        }
        return gatherDataHtml(convertNode(aNode));
    }
    
    function gatherDataText(bNode) {
      if (self.nodeIsLivemarkContainer(bNode))
        return self.livemarks.getSiteURI(bNode.itemId).spec;
      if (self.nodeIsContainer(bNode)) {
        asContainer(bNode);
        var wasOpen = bNode.containerOpen;
        if (!wasOpen)
          bNode.containerOpen = true;

        var childString = bNode.title + NEWLINE;
        var cc = bNode.childCount;
        for (var i = 0; i < cc; ++i) {
          var child = bNode.getChild(i);
          var suffix = i < (cc - 1) ? NEWLINE : "";
          childString += gatherDataText(child) + suffix;
        }
        bNode.containerOpen = wasOpen;
        return childString;
      }
      if (self.nodeIsURI(bNode))
        return (aOverrideURI || bNode.uri);
      if (self.nodeIsSeparator(bNode))
        return "--------------------";
      return "";
    }

    return gatherDataText(convertNode(aNode));
  },

  








  unwrapNodes: function PU_unwrapNodes(blob, type) {
    
    var nodes = [];
    switch(type) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        var JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
        nodes = JSON.decode("[" + blob + "]");
        break;
      case this.TYPE_X_MOZ_URL:
        var parts = blob.split("\n");
        
        
        
        if (parts.length != 1 && parts.length % 2)
          break;
        for (var i = 0; i < parts.length; i=i+2) {
          var uriString = parts[i];
          var titleString = "";
          if (parts.length > i+1)
            titleString = parts[i+1];
          else {
            
            try {
              titleString = this._uri(uriString).QueryInterface(Ci.nsIURL)
                              .fileName;
            }
            catch (e) {}
          }
          
          if (this._uri(uriString)) {
            nodes.push({ uri: uriString,
                         title: titleString ? titleString : uriString ,
                         type: this.TYPE_X_MOZ_URL });
          }
        }
        break;
      case this.TYPE_UNICODE:
        var parts = blob.split("\n");
        for (var i = 0; i < parts.length; i++) {
          var uriString = parts[i];
          
          
          if (uriString.substr(0, 1) == '\x23')
            continue;
          
          if (uriString != "" && this._uri(uriString))
            nodes.push({ uri: uriString,
                         title: uriString,
                         type: this.TYPE_X_MOZ_URL });
        }
        break;
      default:
        LOG("Cannot unwrap data of type " + type);
        throw Cr.NS_ERROR_INVALID_ARG;
    }
    return nodes;
  },

  













  getFolderContents:
  function PU_getFolderContents(aFolderId, aExcludeItems, aExpandQueries) {
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    var options = this.history.getNewQueryOptions();
    options.excludeItems = aExcludeItems;
    options.expandQueries = aExpandQueries;

    var result = this.history.executeQuery(query, options);
    result.root.containerOpen = true;
    return result;
  },

  







  getAnnotationsForURI: function PU_getAnnotationsForURI(aURI) {
    var annosvc = this.annotations;
    var annos = [], val = null;
    var annoNames = annosvc.getPageAnnotationNames(aURI, {});
    for (var i = 0; i < annoNames.length; i++) {
      var flags = {}, exp = {}, mimeType = {}, storageType = {};
      annosvc.getPageAnnotationInfo(aURI, annoNames[i], flags, exp, mimeType, storageType);
      if (storageType.value == annosvc.TYPE_BINARY) {
        var data = {}, length = {}, mimeType = {};
        annosvc.getPageAnnotationBinary(aURI, annoNames[i], data, length, mimeType);
        val = data.value;
      }
      else
        val = annosvc.getPageAnnotation(aURI, annoNames[i]);

      annos.push({name: annoNames[i],
                  flags: flags.value,
                  expires: exp.value,
                  mimeType: mimeType.value,
                  type: storageType.value,
                  value: val});
    }
    return annos;
  },

  








  getAnnotationsForItem: function PU_getAnnotationsForItem(aItemId) {
    var annosvc = this.annotations;
    var annos = [], val = null;
    var annoNames = annosvc.getItemAnnotationNames(aItemId, {});
    for (var i = 0; i < annoNames.length; i++) {
      var flags = {}, exp = {}, mimeType = {}, storageType = {};
      annosvc.getItemAnnotationInfo(aItemId, annoNames[i], flags, exp, mimeType, storageType);
      if (storageType.value == annosvc.TYPE_BINARY) {
        var data = {}, length = {}, mimeType = {};
        annosvc.geItemAnnotationBinary(aItemId, annoNames[i], data, length, mimeType);
        val = data.value;
      }
      else
        val = annosvc.getItemAnnotation(aItemId, annoNames[i]);

      annos.push({name: annoNames[i],
                  flags: flags.value,
                  expires: exp.value,
                  mimeType: mimeType.value,
                  type: storageType.value,
                  value: val});
    }
    return annos;
  },

  








  setAnnotationsForURI: function PU_setAnnotationsForURI(aURI, aAnnos) {
    var annosvc = this.annotations;
    aAnnos.forEach(function(anno) {
      var flags = ("flags" in anno) ? anno.flags : 0;
      var expires = ("expires" in anno) ?
        anno.expires : Ci.nsIAnnotationService.EXPIRE_NEVER;
      if (anno.type == annosvc.TYPE_BINARY) {
        annosvc.setPageAnnotationBinary(aURI, anno.name, anno.value,
                                        anno.value.length, anno.mimeType,
                                        flags, expires);
      }
      else
        annosvc.setPageAnnotation(aURI, anno.name, anno.value, flags, expires);
    });
  },

  








  setAnnotationsForItem: function PU_setAnnotationsForItem(aItemId, aAnnos) {
    var annosvc = this.annotations;
    aAnnos.forEach(function(anno) {
      var flags = ("flags" in anno) ? anno.flags : 0;
      var expires = ("expires" in anno) ?
        anno.expires : Ci.nsIAnnotationService.EXPIRE_NEVER;
      if (anno.type == annosvc.TYPE_BINARY) {
        annosvc.setItemAnnotationBinary(aItemId, anno.name, anno.value,
                                        anno.value.length, anno.mimeType,
                                        flags, expires);
      }
      else {
        annosvc.setItemAnnotation(aItemId, anno.name, anno.value, flags,
                                  expires);
      }
    });
  },

  
  get placesRootId() {
    delete this.placesRootId;
    return this.placesRootId = this.bookmarks.placesRoot;
  },

  get bookmarksMenuFolderId() {
    delete this.bookmarksMenuFolderId;
    return this.bookmarksMenuFolderId = this.bookmarks.bookmarksMenuFolder;
  },

  get toolbarFolderId() {
    delete this.toolbarFolderId;
    return this.toolbarFolderId = this.bookmarks.toolbarFolder;
  },

  get tagsFolderId() {
    delete this.tagsFolderId;
    return this.tagsFolderId = this.bookmarks.tagsFolder;
  },

  get unfiledBookmarksFolderId() {
    delete this.unfiledBookmarksFolderId;
    return this.unfiledBookmarksFolderId = this.bookmarks.unfiledBookmarksFolder;
  },

  





  setPostDataForBookmark: function PU_setPostDataForBookmark(aBookmarkId, aPostData) {
    const annos = this.annotations;
    if (aPostData)
      annos.setItemAnnotation(aBookmarkId, POST_DATA_ANNO, aPostData, 
                              0, Ci.nsIAnnotationService.EXPIRE_NEVER);
    else if (annos.itemHasAnnotation(aBookmarkId, POST_DATA_ANNO))
      annos.removeItemAnnotation(aBookmarkId, POST_DATA_ANNO);
  },

  




  getPostDataForBookmark: function PU_getPostDataForBookmark(aBookmarkId) {
    const annos = this.annotations;
    if (annos.itemHasAnnotation(aBookmarkId, POST_DATA_ANNO))
      return annos.getItemAnnotation(aBookmarkId, POST_DATA_ANNO);

    return null;
  },

  




  getURLAndPostDataForKeyword: function PU_getURLAndPostDataForKeyword(aKeyword) {
    var url = null, postdata = null;
    try {
      var uri = this.bookmarks.getURIForKeyword(aKeyword);
      if (uri) {
        url = uri.spec;
        var bookmarks = this.bookmarks.getBookmarkIdsForURI(uri, {});
        for (let i = 0; i < bookmarks.length; i++) {
          var bookmark = bookmarks[i];
          var kw = this.bookmarks.getKeywordForBookmark(bookmark);
          if (kw == aKeyword) {
            postdata = this.getPostDataForBookmark(bookmark);
            break;
          }
        }
      }
    } catch(ex) {}
    return [url, postdata];
  },

  



  getBookmarksForURI:
  function PU_getBookmarksForURI(aURI) {
    var bmkIds = this.bookmarks.getBookmarkIdsForURI(aURI, {});

    
    return bmkIds.filter(function(aID) {
      var parent = this.bookmarks.getFolderIdForItem(aID);
      
      if (this.annotations.itemHasAnnotation(parent, LMANNO_FEEDURI))
        return false;
      var grandparent = this.bookmarks.getFolderIdForItem(parent);
      
      if (grandparent == this.tagsFolderId)
        return false;
      return true;
    }, this);
  },

  



  getMostRecentBookmarkForURI:
  function PU_getMostRecentBookmarkForURI(aURI) {
    var bmkIds = this.bookmarks.getBookmarkIdsForURI(aURI, {});
    for (var i = 0; i < bmkIds.length; i++) {
      
      var bk = bmkIds[i];
      var parent = this.bookmarks.getFolderIdForItem(bk);
      if (parent == this.unfiledBookmarksFolderId)
        return bk;

      var grandparent = this.bookmarks.getFolderIdForItem(parent);
      if (grandparent != this.tagsFolderId &&
          !this.annotations.itemHasAnnotation(parent, LMANNO_FEEDURI))
        return bk;
    }
    return -1;
  },

  getMostRecentFolderForFeedURI:
  function PU_getMostRecentFolderForFeedURI(aURI) {
    var feedSpec = aURI.spec
    var annosvc = this.annotations;
    var livemarks = annosvc.getItemsWithAnnotation(LMANNO_FEEDURI, {});
    for (var i = 0; i < livemarks.length; i++) {
      if (annosvc.getItemAnnotation(livemarks[i], LMANNO_FEEDURI) == feedSpec)
        return livemarks[i];
    }
    return -1;
  },

  
  
  hasChildURIs: function PU_hasChildURIs(aNode) {
    if (!this.nodeIsContainer(aNode))
      return false;

    
    if (this.nodeIsFolder(aNode) && asQuery(aNode).queryOptions.excludeItems) {
      var itemId = PlacesUtils.getConcreteItemId(aNode);
      var contents = this.getFolderContents(itemId, false, false).root;
      for (var i = 0; i < contents.childCount; ++i) {
        var child = contents.getChild(i);
        if (this.nodeIsURI(child))
          return true;
      }
      return false;
    }

    var wasOpen = aNode.containerOpen;
    if (!wasOpen)
      aNode.containerOpen = true;
    var found = false;
    for (var i = 0; i < aNode.childCount && !found; i++) {
      var child = aNode.getChild(i);
      if (this.nodeIsURI(child))
        found = true;
    }
    if (!wasOpen)
      aNode.containerOpen = false;
    return found;
  },

  getURLsForContainerNode: function PU_getURLsForContainerNode(aNode) {
    let urls = [];
    if (this.nodeIsFolder(aNode) && asQuery(aNode).queryOptions.excludeItems) {
      
      var itemId = this.getConcreteItemId(aNode);
      let contents = this.getFolderContents(itemId, false, false).root;
      for (let i = 0; i < contents.childCount; ++i) {
        let child = contents.getChild(i);
        if (this.nodeIsURI(child))
          urls.push({uri: child.uri, isBookmark: this.nodeIsBookmark(child)});
      }
    }
    else {
      let result, oldViewer, wasOpen;
      try {
        let wasOpen = aNode.containerOpen;
        result = aNode.parentResult;
        oldViewer = result.viewer;
        if (!wasOpen) {
          result.viewer = null;
          aNode.containerOpen = true;
        }
        for (let i = 0; i < aNode.childCount; ++i) {
          
          let child = aNode.getChild(i);
          if (this.nodeIsURI(child)) {
            
            
            
            if ((wasOpen && oldViewer && child.viewIndex != -1) ||
                (oldViewer && !oldViewer.collapseDuplicates) ||
                urls.indexOf(child.uri) == -1) {
              urls.push({ uri: child.uri,
                          isBookmark: this.nodeIsBookmark(child) });
            }
          }
        }
        if (!wasOpen)
          aNode.containerOpen = false;
      }
      finally {
        if (!wasOpen)
          result.viewer = oldViewer;
      }
    }

    return urls;
  },

  










  restoreBookmarksFromJSONFile:
  function PU_restoreBookmarksFromJSONFile(aFile, aExcludeItems) {
    
    var stream = Cc["@mozilla.org/network/file-input-stream;1"].
                 createInstance(Ci.nsIFileInputStream);
    stream.init(aFile, 0x01, 0, 0);
    var converted = Cc["@mozilla.org/intl/converter-input-stream;1"].
                    createInstance(Ci.nsIConverterInputStream);
    converted.init(stream, "UTF-8", 1024,
                   Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);

    
    var str = {};
    var jsonStr = "";
    while (converted.readString(4096, str) != 0)
      jsonStr += str.value;
    converted.close();

    if (jsonStr.length == 0)
      return; 

    this.restoreBookmarksFromJSONString(jsonStr, true, aExcludeItems);
  },

  










  restoreBookmarksFromJSONString:
  function PU_restoreBookmarksFromJSONString(aString, aReplace, aExcludeItems) {
    
    var nodes = this.unwrapNodes(aString, this.TYPE_X_MOZ_PLACE_CONTAINER);

    if (nodes.length == 0 || !nodes[0].children ||
        nodes[0].children.length == 0)
      return; 

    
    nodes[0].children.sort(function sortRoots(aNode, bNode) {
      return (aNode.root && aNode.root == "tagsFolder") ? 1 :
              (bNode.root && bNode.root == "tagsFolder") ? -1 : 0;
    });

    var batch = {
      _utils: this,
      nodes: nodes[0].children,
      runBatched: function restore_runBatched() {
        if (aReplace) {
          var excludeItems = aExcludeItems || [];
          
          
          
          var query = this._utils.history.getNewQuery();
          query.setFolders([this._utils.placesRootId], 1);
          var options = this._utils.history.getNewQueryOptions();
          options.expandQueries = false;
          var root = this._utils.history.executeQuery(query, options).root;
          root.containerOpen = true;
          var childIds = [];
          for (var i = 0; i < root.childCount; i++) {
            var childId = root.getChild(i).itemId;
            if (excludeItems.indexOf(childId) == -1)
              childIds.push(childId);
          }
          root.containerOpen = false;

          for (var i = 0; i < childIds.length; i++) {
            var rootItemId = childIds[i];
            if (rootItemId == this._utils.tagsFolderId) {
              
              var tags = this._utils.tagging.allTags;
              var uris = [];
              for (let i in tags) {
                var tagURIs = this._utils.tagging.getURIsForTag(tags[i]);
                for (let j in tagURIs)
                  this._utils.tagging.untagURI(tagURIs[j], [tags[i]]);
              }
            }
            else if ([this._utils.toolbarFolderId,
                      this._utils.unfiledBookmarksFolderId,
                      this._utils.bookmarksMenuFolderId].indexOf(rootItemId) != -1)
              this._utils.bookmarks.removeFolderChildren(rootItemId);
            else
              this._utils.bookmarks.removeItem(rootItemId);
          }
        }

        var searchIds = [];
        var folderIdMap = [];

        this.nodes.forEach(function(node) {
          if (!node.children || node.children.length == 0)
            return; 

          if (node.root) {
            var container = this.placesRootId; 
            switch (node.root) {
              case "bookmarksMenuFolder":
                container = this.bookmarksMenuFolderId;
                break;
              case "tagsFolder":
                container = this.tagsFolderId;
                break;
              case "unfiledBookmarksFolder":
                container = this.unfiledBookmarksFolderId;
                break;
              case "toolbarFolder":
                container = this.toolbarFolderId;
                break;
            }
 
            
            node.children.forEach(function(child) {
              var index = child.index;
              var [folders, searches] = this.importJSONNode(child, container, index);
              folderIdMap = folderIdMap.concat(folders);
              searchIds = searchIds.concat(searches);
            }, this);
          }
          else
            this.importJSONNode(node, this.placesRootId, node.index);

        }, this._utils);

        
        searchIds.forEach(function(aId) {
          var oldURI = this.bookmarks.getBookmarkURI(aId);
          var uri = this._fixupQuery(this.bookmarks.getBookmarkURI(aId),
                                     folderIdMap);
          if (!uri.equals(oldURI))
            this.bookmarks.changeBookmarkURI(aId, uri);
        }, this._utils);
      }
    };

    this.bookmarks.runInBatchMode(batch, null);
  },

  












  importJSONNode: function PU_importJSONNode(aData, aContainer, aIndex) {
    var folderIdMap = [];
    var searchIds = [];
    var id = -1;
    switch (aData.type) {
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        if (aContainer == PlacesUtils.bookmarks.tagsFolder) {
          if (aData.children) {
            aData.children.forEach(function(aChild) {
              this.tagging.tagURI(this._uri(aChild.uri), [aData.title]);
            }, this);
            return [folderIdMap, searchIds];
          }
        }
        else if (aData.livemark && aData.annos) {
          
          var feedURI = null;
          var siteURI = null;
          aData.annos = aData.annos.filter(function(aAnno) {
            if (aAnno.name == LMANNO_FEEDURI) {
              feedURI = this._uri(aAnno.value);
              return false;
            }
            else if (aAnno.name == LMANNO_SITEURI) {
              siteURI = this._uri(aAnno.value);
              return false;
            }
            return true;
          }, this);

          if (feedURI)
            id = this.livemarks.createLivemark(aContainer, aData.title, siteURI, feedURI, aIndex);
        }
        else {
          id = this.bookmarks.createFolder(aContainer, aData.title, aIndex);
          folderIdMap.push([aData.id, id]);
          
          if (aData.children) {
            aData.children.every(function(aChild, aIndex) {
              var [folderIds, searches] = this.importJSONNode(aChild, id, aIndex);
              folderIdMap = folderIdMap.concat(folderIds);
              searchIds = searchIds.concat(searches);
              return true;
            }, this);
          }
        }
        break;
      case this.TYPE_X_MOZ_PLACE:
        id = this.bookmarks.insertBookmark(aContainer, this._uri(aData.uri), aIndex, aData.title);
        if (aData.keyword)
          this.bookmarks.setKeywordForBookmark(id, aData.keyword);
        if (aData.tags) {
          var tags = aData.tags.split(", ");
          if (tags.length)
            this.tagging.tagURI(this._uri(aData.uri), tags);
        }
        if (aData.charset)
          this.history.setCharsetForURI(this._uri(aData.uri), aData.charset);
        if (aData.uri.match(/^place:/))
          searchIds.push(id);
        break;
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
        id = this.bookmarks.insertSeparator(aContainer, aIndex);
        break;
      default:
    }

    
    if (id != -1) {
      this.bookmarks.setItemDateAdded(id, aData.dateAdded);
      this.bookmarks.setItemLastModified(id, aData.lastModified);
      if (aData.annos)
        this.setAnnotationsForItem(id, aData.annos);
    }

    return [folderIdMap, searchIds];
  },

  










  _fixupQuery: function PU__fixupQuery(aQueryURI, aFolderIdMap) {
    var queries = {};
    var options = {};
    this.history.queryStringToQueries(aQueryURI.spec, queries, {}, options);

    var fixedQueries = [];
    queries.value.forEach(function(aQuery) {
      var folders = aQuery.getFolders({});

      var newFolders = [];
      aFolderIdMap.forEach(function(aMapping) {
        if (folders.indexOf(aMapping[0]) != -1)
          newFolders.push(aMapping[1]);
      });

      if (newFolders.length)
        aQuery.setFolders(newFolders, newFolders.length);
      fixedQueries.push(aQuery);
    });

    var stringURI = this.history.queriesToQueryString(fixedQueries,
                                                      fixedQueries.length,
                                                      options.value);
    return this._uri(stringURI);
  },

  

















  serializeNodeAsJSONToOutputStream:
  function PU_serializeNodeAsJSONToOutputStream(aNode, aStream, aIsUICommand,
                                                aResolveShortcuts,
                                                aExcludeItems) {
    var self = this;
    
    function addGenericProperties(aPlacesNode, aJSNode) {
      aJSNode.title = aPlacesNode.title;
      var id = aPlacesNode.itemId;
      if (id != -1) {
        aJSNode.id = id;

        var parent = aPlacesNode.parent;
        if (parent)
          aJSNode.parent = parent.itemId;
        var dateAdded = aPlacesNode.dateAdded;
        if (dateAdded)
          aJSNode.dateAdded = dateAdded;
        var lastModified = aPlacesNode.lastModified;
        if (lastModified)
          aJSNode.lastModified = lastModified;

        
        var annos = [];
        try {
          annos = self.getAnnotationsForItem(id).filter(function(anno) {
            
            
            
            
            if (anno.name == LMANNO_FEEDURI)
              aJSNode.livemark = 1;
            if (anno.name == READ_ONLY_ANNO && aResolveShortcuts) {
              
              return false;
            }
            return true;
          });
        } catch(ex) {
          LOG(ex);
        }
        if (annos.length != 0)
          aJSNode.annos = annos;
      }
      
    }

    function addURIProperties(aPlacesNode, aJSNode) {
      aJSNode.type = self.TYPE_X_MOZ_PLACE;
      aJSNode.uri = aPlacesNode.uri;
      if (aJSNode.id && aJSNode.id != -1) {
        
        var keyword = self.bookmarks.getKeywordForBookmark(aJSNode.id);
        if (keyword)
          aJSNode.keyword = keyword;
      }

      var tags = aIsUICommand ? aPlacesNode.tags : null;
      if (tags)
        aJSNode.tags = tags;

      
      var uri = self._uri(aPlacesNode.uri);
      var lastCharset = self.history.getCharsetForURI(uri);
      if (lastCharset)
        aJSNode.charset = lastCharset;
    }

    function addSeparatorProperties(aPlacesNode, aJSNode) {
      aJSNode.type = self.TYPE_X_MOZ_PLACE_SEPARATOR;
    }

    function addContainerProperties(aPlacesNode, aJSNode) {
      
      var concreteId = PlacesUtils.getConcreteItemId(aPlacesNode);
      if (aJSNode.id != -1 && (PlacesUtils.nodeIsQuery(aPlacesNode) ||
          (concreteId != aPlacesNode.itemId && !aResolveShortcuts))) {
        aJSNode.type = self.TYPE_X_MOZ_PLACE;
        aJSNode.uri = aPlacesNode.uri;
        
        if (aIsUICommand)
          aJSNode.concreteId = concreteId;
        return;
      }
      else if (aJSNode.id != -1) { 
        if (concreteId != aPlacesNode.itemId)
        aJSNode.type = self.TYPE_X_MOZ_PLACE;
        aJSNode.type = self.TYPE_X_MOZ_PLACE_CONTAINER;
        
        if (aJSNode.id == self.bookmarks.placesRoot)
          aJSNode.root = "placesRoot";
        else if (aJSNode.id == self.bookmarks.bookmarksMenuFolder)
          aJSNode.root = "bookmarksMenuFolder";
        else if (aJSNode.id == self.bookmarks.tagsFolder)
          aJSNode.root = "tagsFolder";
        else if (aJSNode.id == self.bookmarks.unfiledBookmarksFolder)
          aJSNode.root = "unfiledBookmarksFolder";
        else if (aJSNode.id == self.bookmarks.toolbarFolder)
          aJSNode.root = "toolbarFolder";
      }
    }

    function writeScalarNode(aStream, aNode) {
      
      var jstr = self.toJSONString(aNode);
      
      aStream.write(jstr, jstr.length);
    }

    function writeComplexNode(aStream, aNode, aSourceNode) {
      var escJSONStringRegExp = /(["\\])/g;
      
      var properties = [];
      for (let [name, value] in Iterator(aNode)) {
        if (name == "annos")
          value = self.toJSONString(value);
        else if (typeof value == "string")
          value = "\"" + value.replace(escJSONStringRegExp, '\\$1') + "\"";
        properties.push("\"" + name.replace(escJSONStringRegExp, '\\$1') + "\":" + value);
      }
      var jStr = "{" + properties.join(",") + ",\"children\":[";
      aStream.write(jStr, jStr.length);

      
      if (!aNode.livemark) {
        asContainer(aSourceNode);
        var wasOpen = aSourceNode.containerOpen;
        if (!wasOpen)
          aSourceNode.containerOpen = true;
        var cc = aSourceNode.childCount;
        for (var i = 0; i < cc; ++i) {
          var childNode = aSourceNode.getChild(i);
          if (aExcludeItems && aExcludeItems.indexOf(childNode.itemId) != -1)
            continue;
          if (i != 0)
            aStream.write(",", 1);
          serializeNodeToJSONStream(aSourceNode.getChild(i), i);
        }
        if (!wasOpen)
          aSourceNode.containerOpen = false;
      }

      
      aStream.write("]}", 2);
    }

    function serializeNodeToJSONStream(bNode, aIndex) {
      var node = {};

      
      
      
      if (aIndex)
        node.index = aIndex;

      addGenericProperties(bNode, node);

      if (self.nodeIsURI(bNode)) {
        
        
        
        try {
          self._uri(bNode.uri);
        } catch (ex) {
          return;
        }
        addURIProperties(bNode, node);
      }
      else if (self.nodeIsContainer(bNode))
        addContainerProperties(bNode, node);
      else if (self.nodeIsSeparator(bNode))
        addSeparatorProperties(bNode, node);

      if (!node.feedURI && node.type == self.TYPE_X_MOZ_PLACE_CONTAINER)
        writeComplexNode(aStream, node, bNode);
      else
        writeScalarNode(aStream, node);
    }

    
    serializeNodeToJSONStream(aNode, null);
  },

  
  toJSONString: function PU_toJSONString(aObj) {
    var JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    return JSON.encode(aObj);
  },

  


  backupBookmarksToFile: function PU_backupBookmarksToFile(aFile, aExcludeItems) {
    if (aFile.exists() && !aFile.isWritable())
      return; 

    
    var stream = Cc["@mozilla.org/network/file-output-stream;1"].
                 createInstance(Ci.nsIFileOutputStream);
    stream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

    
    var converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
                 createInstance(Ci.nsIConverterOutputStream);
    converter.init(stream, "UTF-8", 0, 0x0000);

    
    var streamProxy = {
      converter: converter,
      write: function(aData, aLen) {
        this.converter.writeString(aData);
      }
    };

    
    var options = this.history.getNewQueryOptions();
    options.expandQueries = false;
    var query = this.history.getNewQuery();
    query.setFolders([this.bookmarks.placesRoot], 1);
    var result = this.history.executeQuery(query, options);
    result.root.containerOpen = true;
    
    this.serializeNodeAsJSONToOutputStream(result.root, streamProxy,
                                           false, false, aExcludeItems);
    result.root.containerOpen = false;

    
    converter.close();
    stream.close();
  },

  










  archiveBookmarksFile:
  function PU_archiveBookmarksFile(aNumberOfBackups, aForceArchive) {
    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    var bookmarksBackupDir = dirService.get("ProfD", Ci.nsILocalFile);
    bookmarksBackupDir.append("bookmarkbackups");
    if (!bookmarksBackupDir.exists()) {
      bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
      if (!bookmarksBackupDir.exists())
        return; 
    }

    
    
    
    var date = new Date().toLocaleFormat("%Y-%m-%d");
    var backupFilename = this.getFormattedString("bookmarksArchiveFilename", [date]);

    var backupFile = null;
    if (!aForceArchive) {
      var backupFileNames = [];
      var backupFilenamePrefix = backupFilename.substr(0, backupFilename.indexOf("-"));
      var entries = bookmarksBackupDir.directoryEntries;
      while (entries.hasMoreElements()) {
        var entry = entries.getNext().QueryInterface(Ci.nsIFile);
        var backupName = entry.leafName;
        if (backupName.substr(0, backupFilenamePrefix.length) == backupFilenamePrefix) {
          if (backupName == backupFilename)
            backupFile = entry;
          backupFileNames.push(backupName);
        }
      }

      var numberOfBackupsToDelete = 0;
      if (aNumberOfBackups > -1)
        numberOfBackupsToDelete = backupFileNames.length - aNumberOfBackups;

      if (numberOfBackupsToDelete > 0) {
        
        
        
        if (!backupFile)
          numberOfBackupsToDelete++;

        backupFileNames.sort();
        while (numberOfBackupsToDelete--) {
          let backupFile = bookmarksBackupDir.clone();
          backupFile.append(backupFileNames[0]);
          backupFile.remove(false);
          backupFileNames.shift();
        }
      }

      
      
      if (backupFile || aNumberOfBackups == 0)
        return;
    }

    backupFile = bookmarksBackupDir.clone();
    backupFile.append(backupFilename);

    if (aForceArchive && backupFile.exists())
        backupFile.remove(false);

    if (!backupFile.exists())
      backupFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);

    this.backupBookmarksToFile(backupFile);
  },

  



  getMostRecentBackup: function PU_getMostRecentBackup() {
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    var bookmarksBackupDir = dirService.get("ProfD", Ci.nsILocalFile);
    bookmarksBackupDir.append("bookmarkbackups");
    if (!bookmarksBackupDir.exists())
      return null;

    var backups = [];
    var entries = bookmarksBackupDir.directoryEntries;
    while (entries.hasMoreElements()) {
      var entry = entries.getNext().QueryInterface(Ci.nsIFile);
      if (!entry.isHidden() && entry.leafName.match(/^bookmarks-.+(html|json)?$/))
        backups.push(entry.leafName);
    }

    if (backups.length ==  0)
      return null;

    backups.sort();
    var filename = backups.pop();

    var backupFile = bookmarksBackupDir.clone();
    backupFile.append(filename);
    return backupFile;
  }
};
