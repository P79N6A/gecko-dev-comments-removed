




this.EXPORTED_SYMBOLS = [
  "PlacesUtils"
, "PlacesAggregatedTransaction"
, "PlacesCreateFolderTransaction"
, "PlacesCreateBookmarkTransaction"
, "PlacesCreateSeparatorTransaction"
, "PlacesCreateLivemarkTransaction"
, "PlacesMoveItemTransaction"
, "PlacesRemoveItemTransaction"
, "PlacesEditItemTitleTransaction"
, "PlacesEditBookmarkURITransaction"
, "PlacesSetItemAnnotationTransaction"
, "PlacesSetPageAnnotationTransaction"
, "PlacesEditBookmarkKeywordTransaction"
, "PlacesEditBookmarkPostDataTransaction"
, "PlacesEditItemDateAddedTransaction"
, "PlacesEditItemLastModifiedTransaction"
, "PlacesSortFolderByNameTransaction"
, "PlacesTagURITransaction"
, "PlacesUntagURITransaction"
];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
                                  "resource://gre/modules/Deprecated.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BookmarkJSONUtils",
                                  "resource://gre/modules/BookmarkJSONUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
                                  "resource://gre/modules/PlacesBackups.jsm");




const MIN_TRANSACTIONS_FOR_BATCH = 5;

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
function asVisit(aNode) {
  Deprecated.warning(
    "asVisit is deprecated and will be removed in a future version",
    "https://bugzilla.mozilla.org/show_bug.cgi?id=561450");
  return aNode;
};
function asContainer(aNode) QI_node(aNode, Ci.nsINavHistoryContainerResultNode);
function asQuery(aNode) QI_node(aNode, Ci.nsINavHistoryQueryResultNode);

this.PlacesUtils = {
  
  TYPE_X_MOZ_PLACE_CONTAINER: "text/x-moz-place-container",
  
  TYPE_X_MOZ_PLACE_SEPARATOR: "text/x-moz-place-separator",
  
  TYPE_X_MOZ_PLACE: "text/x-moz-place",
  
  TYPE_X_MOZ_URL: "text/x-moz-url",
  
  TYPE_HTML: "text/html",
  
  TYPE_UNICODE: "text/unicode",
  
  TYPE_X_MOZ_PLACE_ACTION: "text/x-moz-place-action",

  EXCLUDE_FROM_BACKUP_ANNO: "places/excludeFromBackup",
  LMANNO_FEEDURI: "livemark/feedURI",
  LMANNO_SITEURI: "livemark/siteURI",
  POST_DATA_ANNO: "bookmarkProperties/POSTData",
  READ_ONLY_ANNO: "placesInternal/READ_ONLY",
  CHARSET_ANNO: "URIProperties/characterSet",

  TOPIC_SHUTDOWN: "places-shutdown",
  TOPIC_INIT_COMPLETE: "places-init-complete",
  TOPIC_DATABASE_LOCKED: "places-database-locked",
  TOPIC_EXPIRATION_FINISHED: "places-expiration-finished",
  TOPIC_FEEDBACK_UPDATED: "places-autocomplete-feedback-updated",
  TOPIC_FAVICONS_EXPIRED: "places-favicons-expired",
  TOPIC_VACUUM_STARTING: "places-vacuum-starting",
  TOPIC_BOOKMARKS_RESTORE_BEGIN: "bookmarks-restore-begin",
  TOPIC_BOOKMARKS_RESTORE_SUCCESS: "bookmarks-restore-success",
  TOPIC_BOOKMARKS_RESTORE_FAILED: "bookmarks-restore-failed",

  asVisit: function(aNode) asVisit(aNode),
  asContainer: function(aNode) asContainer(aNode),
  asQuery: function(aNode) asQuery(aNode),

  endl: NEWLINE,

  





  _uri: function PU__uri(aSpec) {
    return NetUtil.newURI(aSpec);
  },

  





  toISupportsString: function PU_toISupportsString(aString) {
    let s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PU_getString(key) {
    return bundle.GetStringFromName(key);
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
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR;
  },

  





  nodeIsVisit: function PU_nodeIsVisit(aNode) {
    Deprecated.warning(
      "nodeIsVisit is deprecated ans will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=561450");
    return this.nodeIsURI(aNode) && aNode.parent &&
           this.nodeIsQuery(aNode.parent) &&
           asQuery(aNode.parent).queryOptions.resultType ==
             Ci.nsINavHistoryQueryOptions.RESULTS_AS_VISIT;
  },

  





  get uriTypes() {
    Deprecated.warning(
      "uriTypes is deprecated ans will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=561450");
    return [Ci.nsINavHistoryResultNode.RESULT_TYPE_URI];
  },
  nodeIsURI: function PU_nodeIsURI(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI;
  },

  





  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  




  nodeAncestors: function PU_nodeAncestors(aNode) {
    let node = aNode.parent;
    while (node) {
      yield node;
      node = node.parent;
    }
  },

  













  get _readOnly() {
    
    this.annotations.addObserver(this, false);
    this.registerShutdownFunction(function () {
      this.annotations.removeObserver(this);
    });

    var readOnly = this.annotations.getItemsWithAnnotation(this.READ_ONLY_ANNO);
    this.__defineGetter__("_readOnly", function() readOnly);
    return this._readOnly;
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAnnotationObserver
  , Ci.nsIObserver
  , Ci.nsITransactionListener
  ]),

  _shutdownFunctions: [],
  registerShutdownFunction: function PU_registerShutdownFunction(aFunc)
  {
    
    if (this._shutdownFunctions.length == 0) {
      Services.obs.addObserver(this, this.TOPIC_SHUTDOWN, false);
    }
    this._shutdownFunctions.push(aFunc);
  },

  
  
  observe: function PU_observe(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case this.TOPIC_SHUTDOWN:
        Services.obs.removeObserver(this, this.TOPIC_SHUTDOWN);
        while (this._shutdownFunctions.length > 0) {
          this._shutdownFunctions.shift().apply(this);
        }
        if (this._bookmarksServiceObserversQueue.length > 0) {
          
          this._bookmarksServiceObserversQueue.length = 0;
        }
        break;
      case "bookmarks-service-ready":
        this._bookmarksServiceReady = true;
        while (this._bookmarksServiceObserversQueue.length > 0) {
          let observerInfo = this._bookmarksServiceObserversQueue.shift();
          this.bookmarks.addObserver(observerInfo.observer, observerInfo.weak);
        }
        break;
    }
  },

  
  

  onItemAnnotationSet: function PU_onItemAnnotationSet(aItemId, aAnnotationName)
  {
    if (aAnnotationName == this.READ_ONLY_ANNO &&
        this._readOnly.indexOf(aItemId) == -1)
      this._readOnly.push(aItemId);
  },

  onItemAnnotationRemoved:
  function PU_onItemAnnotationRemoved(aItemId, aAnnotationName)
  {
    var index = this._readOnly.indexOf(aItemId);
    if (aAnnotationName == this.READ_ONLY_ANNO && index > -1)
      delete this._readOnly[index];
  },

  onPageAnnotationSet: function() {},
  onPageAnnotationRemoved: function() {},


  
  

  didDo: function PU_didDo(aManager, aTransaction, aDoResult)
  {
    updateCommandsOnActiveWindow();
  },

  didUndo: function PU_didUndo(aManager, aTransaction, aUndoResult)
  {
    updateCommandsOnActiveWindow();
  },

  didRedo: function PU_didRedo(aManager, aTransaction, aRedoResult)
  {
    updateCommandsOnActiveWindow();
  },

  didBeginBatch: function PU_didBeginBatch(aManager, aResult)
  {
    
    
    
    
    
    
    
    
    this.transactionManager.doTransaction({ doTransaction: function() {},
                                            undoTransaction: function() {},
                                            redoTransaction: function() {},
                                            isTransient: false,
                                            merge: function() { return false; }
                                          });
  },

  willDo: function PU_willDo() {},
  willUndo: function PU_willUndo() {},
  willRedo: function PU_willRedo() {},
  willBeginBatch: function PU_willBeginBatch() {},
  willEndBatch: function PU_willEndBatch() {},
  didEndBatch: function PU_didEndBatch() {},
  willMerge: function PU_willMerge() {},
  didMerge: function PU_didMerge() {},


  






  nodeIsReadOnly: function PU_nodeIsReadOnly(aNode) {
    let itemId = aNode.itemId;
    if (itemId != -1) {
      return this._readOnly.indexOf(itemId) != -1;
    }

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
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY],
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

  





  isReadonlyFolder: function(aNode) {
    return this.nodeIsFolder(aNode) &&
           this._readOnly.indexOf(asQuery(aNode).folderItemId) != -1;
  },

  



  getConcreteItemId: function PU_getConcreteItemId(aNode) {
    if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT)
      return asQuery(aNode).folderItemId;
    else if (PlacesUtils.nodeIsTagQuery(aNode)) {
      
      
      var queries = aNode.getQueries();
      var folders = queries[0].getFolders();
      return folders[0];
    }
    return aNode.itemId;
  },

  














  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI, aForceCopy) {
    
    
    
    
    
    function convertNode(cNode) {
      if (PlacesUtils.nodeIsFolder(cNode) &&
          asQuery(cNode).queryOptions.excludeItems) {
        let concreteId = PlacesUtils.getConcreteItemId(cNode);
        return [PlacesUtils.getFolderContents(concreteId, false, true).root, true];
      }

      
      return [cNode, false];
    }

    function gatherLivemarkUrl(aNode) {
      try {
        return PlacesUtils.annotations
                          .getItemAnnotation(aNode.itemId,
                                             PlacesUtils.LMANNO_SITEURI);
      } catch (ex) {
        return PlacesUtils.annotations
                          .getItemAnnotation(aNode.itemId,
                                             PlacesUtils.LMANNO_FEEDURI);
      }
    }

    function isLivemark(aNode) {
      return PlacesUtils.nodeIsFolder(aNode) &&
             PlacesUtils.annotations
                        .itemHasAnnotation(aNode.itemId,
                                           PlacesUtils.LMANNO_FEEDURI);
    }

    switch (aType) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER: {
        let writer = {
          value: "",
          write: function PU_wrapNode__write(aStr, aLen) {
            this.value += aStr;
          }
        };

        let [node, shouldClose] = convertNode(aNode);
        this._serializeNodeAsJSONToOutputStream(node, writer, true, aForceCopy);
        if (shouldClose)
          node.containerOpen = false;

        return writer.value;
      }
      case this.TYPE_X_MOZ_URL: {
        function gatherDataUrl(bNode) {
          if (isLivemark(bNode)) {
            return gatherLivemarkUrl(bNode) + NEWLINE + bNode.title;
          }

          if (PlacesUtils.nodeIsURI(bNode))
            return (aOverrideURI || bNode.uri) + NEWLINE + bNode.title;
          
          return "";
        }

        let [node, shouldClose] = convertNode(aNode);
        let dataUrl = gatherDataUrl(node);
        if (shouldClose)
          node.containerOpen = false;

        return dataUrl;
      }
      case this.TYPE_HTML: {
        function gatherDataHtml(bNode) {
          function htmlEscape(s) {
            s = s.replace(/&/g, "&amp;");
            s = s.replace(/>/g, "&gt;");
            s = s.replace(/</g, "&lt;");
            s = s.replace(/"/g, "&quot;");
            s = s.replace(/'/g, "&apos;");
            return s;
          }
          
          let escapedTitle = bNode.title ? htmlEscape(bNode.title) : "";

          if (isLivemark(bNode)) {
            return "<A HREF=\"" + gatherLivemarkUrl(bNode) + "\">" + escapedTitle + "</A>" + NEWLINE;
          }

          if (PlacesUtils.nodeIsContainer(bNode)) {
            asContainer(bNode);
            let wasOpen = bNode.containerOpen;
            if (!wasOpen)
              bNode.containerOpen = true;

            let childString = "<DL><DT>" + escapedTitle + "</DT>" + NEWLINE;
            let cc = bNode.childCount;
            for (let i = 0; i < cc; ++i)
              childString += "<DD>"
                             + NEWLINE
                             + gatherDataHtml(bNode.getChild(i))
                             + "</DD>"
                             + NEWLINE;
            bNode.containerOpen = wasOpen;
            return childString + "</DL>" + NEWLINE;
          }
          if (PlacesUtils.nodeIsURI(bNode))
            return "<A HREF=\"" + bNode.uri + "\">" + escapedTitle + "</A>" + NEWLINE;
          if (PlacesUtils.nodeIsSeparator(bNode))
            return "<HR>" + NEWLINE;
          return "";
        }

        let [node, shouldClose] = convertNode(aNode);
        let dataHtml = gatherDataHtml(node);
        if (shouldClose)
          node.containerOpen = false;

        return dataHtml;
      }
    }

    
    function gatherDataText(bNode) {
      if (isLivemark(bNode)) {
        return gatherLivemarkUrl(bNode);
      }

      if (PlacesUtils.nodeIsContainer(bNode)) {
        asContainer(bNode);
        let wasOpen = bNode.containerOpen;
        if (!wasOpen)
          bNode.containerOpen = true;

        let childString = bNode.title + NEWLINE;
        let cc = bNode.childCount;
        for (let i = 0; i < cc; ++i) {
          let child = bNode.getChild(i);
          let suffix = i < (cc - 1) ? NEWLINE : "";
          childString += gatherDataText(child) + suffix;
        }
        bNode.containerOpen = wasOpen;
        return childString;
      }
      if (PlacesUtils.nodeIsURI(bNode))
        return (aOverrideURI || bNode.uri);
      if (PlacesUtils.nodeIsSeparator(bNode))
        return "--------------------";
      return "";
    }

    let [node, shouldClose] = convertNode(aNode);
    let dataText = gatherDataText(node);
    
    if (shouldClose)
      node.containerOpen = false;

    return dataText;
  },

  








  unwrapNodes: function PU_unwrapNodes(blob, type) {
    
    var nodes = [];
    switch(type) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        nodes = JSON.parse("[" + blob + "]");
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
    var annoNames = annosvc.getPageAnnotationNames(aURI);
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
    var annoNames = annosvc.getItemAnnotationNames(aItemId);
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
      if (!anno.value) {
        annosvc.removePageAnnotation(aURI, anno.name);
        return;
      }
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
      if (!anno.value) {
        annosvc.removeItemAnnotation(aItemId, anno.name);
        return;
      }
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

  






  isRootItem: function PU_isRootItem(aItemId) {
    return aItemId == PlacesUtils.bookmarksMenuFolderId ||
           aItemId == PlacesUtils.toolbarFolderId ||
           aItemId == PlacesUtils.unfiledBookmarksFolderId ||
           aItemId == PlacesUtils.tagsFolderId ||
           aItemId == PlacesUtils.placesRootId;
  },

  





  setPostDataForBookmark: function PU_setPostDataForBookmark(aBookmarkId, aPostData) {
    const annos = this.annotations;
    if (aPostData)
      annos.setItemAnnotation(aBookmarkId, this.POST_DATA_ANNO, aPostData, 
                              0, Ci.nsIAnnotationService.EXPIRE_NEVER);
    else if (annos.itemHasAnnotation(aBookmarkId, this.POST_DATA_ANNO))
      annos.removeItemAnnotation(aBookmarkId, this.POST_DATA_ANNO);
  },

  




  getPostDataForBookmark: function PU_getPostDataForBookmark(aBookmarkId) {
    const annos = this.annotations;
    if (annos.itemHasAnnotation(aBookmarkId, this.POST_DATA_ANNO))
      return annos.getItemAnnotation(aBookmarkId, this.POST_DATA_ANNO);

    return null;
  },

  




  getURLAndPostDataForKeyword: function PU_getURLAndPostDataForKeyword(aKeyword) {
    var url = null, postdata = null;
    try {
      var uri = this.bookmarks.getURIForKeyword(aKeyword);
      if (uri) {
        url = uri.spec;
        var bookmarks = this.bookmarks.getBookmarkIdsForURI(uri);
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
    var bmkIds = this.bookmarks.getBookmarkIdsForURI(aURI);

    
    return bmkIds.filter(function(aID) {
      var parentId = this.bookmarks.getFolderIdForItem(aID);
      var grandparentId = this.bookmarks.getFolderIdForItem(parentId);
      
      if (grandparentId == this.tagsFolderId)
        return false;
      return true;
    }, this);
  },

  







  getMostRecentBookmarkForURI:
  function PU_getMostRecentBookmarkForURI(aURI) {
    var bmkIds = this.bookmarks.getBookmarkIdsForURI(aURI);
    for (var i = 0; i < bmkIds.length; i++) {
      
      var itemId = bmkIds[i];
      var parentId = this.bookmarks.getFolderIdForItem(itemId);
      
      
      if (parentId == this.unfiledBookmarksFolderId ||
          parentId == this.toolbarFolderId ||
          parentId == this.bookmarksMenuFolderId)
        return itemId;

      var grandparentId = this.bookmarks.getFolderIdForItem(parentId);
      if (grandparentId != this.tagsFolderId)
        return itemId;
    }
    return -1;
  },

  
















  getContainerNodeWithOptions:
  function PU_getContainerNodeWithOptions(aNode, aExcludeItems, aExpandQueries) {
    if (!this.nodeIsContainer(aNode))
      throw Cr.NS_ERROR_INVALID_ARG;

    
    var excludeItems = asQuery(aNode).queryOptions.excludeItems ||
                       asQuery(aNode.parentResult.root).queryOptions.excludeItems;
    
    var expandQueries = asQuery(aNode).queryOptions.expandQueries &&
                        asQuery(aNode.parentResult.root).queryOptions.expandQueries;

    
    if (excludeItems == aExcludeItems && expandQueries == aExpandQueries)
      return aNode;

    
    var queries = {}, options = {};
    this.history.queryStringToQueries(aNode.uri, queries, {}, options);
    options.value.excludeItems = aExcludeItems;
    options.value.expandQueries = aExpandQueries;
    return this.history.executeQueries(queries.value,
                                       queries.value.length,
                                       options.value).root;
  },

  






  hasChildURIs: function PU_hasChildURIs(aNode) {
    if (!this.nodeIsContainer(aNode))
      return false;

    let root = this.getContainerNodeWithOptions(aNode, false, true);
    let result = root.parentResult;
    let didSuppressNotifications = false;
    let wasOpen = root.containerOpen;
    if (!wasOpen) {
      didSuppressNotifications = result.suppressNotifications;
      if (!didSuppressNotifications)
        result.suppressNotifications = true;

      root.containerOpen = true;
    }

    let found = false;
    for (let i = 0; i < root.childCount && !found; i++) {
      let child = root.getChild(i);
      if (this.nodeIsURI(child))
        found = true;
    }

    if (!wasOpen) {
      root.containerOpen = false;
      if (!didSuppressNotifications)
        result.suppressNotifications = false;
    }
    return found;
  },

  







  getURLsForContainerNode: function PU_getURLsForContainerNode(aNode) {
    let urls = [];
    if (!this.nodeIsContainer(aNode))
      return urls;

    let root = this.getContainerNodeWithOptions(aNode, false, true);
    let result = root.parentResult;
    let wasOpen = root.containerOpen;
    let didSuppressNotifications = false;
    if (!wasOpen) {
      didSuppressNotifications = result.suppressNotifications;
      if (!didSuppressNotifications)
        result.suppressNotifications = true;

      root.containerOpen = true;
    }

    for (let i = 0; i < root.childCount; ++i) {
      let child = root.getChild(i);
      if (this.nodeIsURI(child))
        urls.push({uri: child.uri, isBookmark: this.nodeIsBookmark(child)});
    }

    if (!wasOpen) {
      root.containerOpen = false;
      if (!didSuppressNotifications)
        result.suppressNotifications = false;
    }
    return urls;
  },

  












  importJSONNode: function PU_importJSONNode(aData, aContainer, aIndex, aGrandParentId) {
    Deprecated.warning(
      "importJSONNode is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=855842");

    var folderIdMap = [];
    var searchIds = [];
    var id = -1;
    switch (aData.type) {
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        if (aContainer == PlacesUtils.tagsFolderId) {
          
          if (aData.children) {
            aData.children.forEach(function(aChild) {
              try {
                this.tagging.tagURI(this._uri(aChild.uri), [aData.title]);
              } catch (ex) {
                
              }
            }, this);
            return [folderIdMap, searchIds];
          }
        }
        else if (aData.livemark && aData.annos) {
          
          var feedURI = null;
          var siteURI = null;
          aData.annos = aData.annos.filter(function(aAnno) {
            switch (aAnno.name) {
              case this.LMANNO_FEEDURI:
                feedURI = this._uri(aAnno.value);
                return false;
              case this.LMANNO_SITEURI:
                siteURI = this._uri(aAnno.value);
                return false;
              default:
                return true;
            }
          }, this);

          if (feedURI) {
            this.livemarks.addLivemark(
              { title: aData.title
              , feedURI: feedURI
              , parentId: aContainer
              , index: aIndex
              , lastModified: aData.lastModified
              , siteURI: siteURI
              },
              (function(aStatus, aLivemark) {
                if (Components.isSuccessCode(aStatus)) {
                  let id = aLivemark.id;
                  if (aData.dateAdded)
                    this.bookmarks.setItemDateAdded(id, aData.dateAdded);
                  if (aData.annos && aData.annos.length)
                    this.setAnnotationsForItem(id, aData.annos);
                }
              }).bind(this)
            );
          }
        }
        else {
          id = this.bookmarks.createFolder(aContainer, aData.title, aIndex);
          folderIdMap[aData.id] = id;
          
          if (aData.children) {
            aData.children.forEach(function(aChild, aIndex) {
              var [folders, searches] = this.importJSONNode(aChild, id, aIndex, aContainer);
              for (var i = 0; i < folders.length; i++) {
                if (folders[i])
                  folderIdMap[i] = folders[i];
              }
              searchIds = searchIds.concat(searches);
            }, this);
          }
        }
        break;
      case this.TYPE_X_MOZ_PLACE:
        id = this.bookmarks.insertBookmark(aContainer, this._uri(aData.uri),
                                           aIndex, aData.title);
        if (aData.keyword)
          this.bookmarks.setKeywordForBookmark(id, aData.keyword);
        if (aData.tags) {
          var tags = aData.tags.split(", ");
          if (tags.length)
            this.tagging.tagURI(this._uri(aData.uri), tags);
        }
        if (aData.charset) {
            this.setCharsetForURI(this._uri(aData.uri), aData.charset);
        }
        if (aData.uri.substr(0, 6) == "place:")
          searchIds.push(id);
        if (aData.icon) {
          try {
            
            let faviconURI = this._uri("fake-favicon-uri:" + aData.uri);
            this.favicons.replaceFaviconDataFromDataURL(faviconURI, aData.icon, 0);
            this.favicons.setAndFetchFaviconForPage(this._uri(aData.uri), faviconURI, false,
              this.favicons.FAVICON_LOAD_NON_PRIVATE);
          } catch (ex) {
            Components.utils.reportError("Failed to import favicon data:"  + ex);
          }
        }
        if (aData.iconUri) {
          try {
            this.favicons.setAndFetchFaviconForPage(this._uri(aData.uri),
                                                    this._uri(aData.iconUri),
                                                    false,
                                                    this.favicons.FAVICON_LOAD_NON_PRIVATE);
          } catch (ex) {
            Components.utils.reportError("Failed to import favicon URI:"  + ex);
          }
        }
        break;
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
        id = this.bookmarks.insertSeparator(aContainer, aIndex);
        break;
      default:
        
    }

    
    if (id != -1 &&
        aContainer != PlacesUtils.tagsFolderId &&
        aGrandParentId != PlacesUtils.tagsFolderId) {
      if (aData.dateAdded)
        this.bookmarks.setItemDateAdded(id, aData.dateAdded);
      if (aData.lastModified)
        this.bookmarks.setItemLastModified(id, aData.lastModified);
      if (aData.annos && aData.annos.length)
        this.setAnnotationsForItem(id, aData.annos);
    }

    return [folderIdMap, searchIds];
  },

  

















  _serializeNodeAsJSONToOutputStream:
  function PU__serializeNodeAsJSONToOutputStream(aNode, aStream, aIsUICommand,
                                                aResolveShortcuts,
                                                aExcludeItems) {
    function addGenericProperties(aPlacesNode, aJSNode) {
      aJSNode.title = aPlacesNode.title;
      aJSNode.id = aPlacesNode.itemId;
      if (aJSNode.id != -1) {
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
          annos = PlacesUtils.getAnnotationsForItem(aJSNode.id).filter(function(anno) {
            
            
            
            
            if (anno.name == PlacesUtils.LMANNO_FEEDURI)
              aJSNode.livemark = 1;
            if (anno.name == PlacesUtils.READ_ONLY_ANNO && aResolveShortcuts) {
              
              return false;
            }
            return true;
          });
        } catch(ex) {}
        if (annos.length != 0)
          aJSNode.annos = annos;
      }
      
    }

    function addURIProperties(aPlacesNode, aJSNode) {
      aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
      aJSNode.uri = aPlacesNode.uri;
      if (aJSNode.id && aJSNode.id != -1) {
        
        var keyword = PlacesUtils.bookmarks.getKeywordForBookmark(aJSNode.id);
        if (keyword)
          aJSNode.keyword = keyword;
      }

      var tags = aIsUICommand ? aPlacesNode.tags : null;
      if (tags)
        aJSNode.tags = tags;

      
      var uri = PlacesUtils._uri(aPlacesNode.uri);
      try {
        var lastCharset = PlacesUtils.annotations.getPageAnnotation(
                            uri, PlacesUtils.CHARSET_ANNO);
        aJSNode.charset = lastCharset;
      } catch (e) {}
    }

    function addSeparatorProperties(aPlacesNode, aJSNode) {
      aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR;
    }

    function addContainerProperties(aPlacesNode, aJSNode) {
      var concreteId = PlacesUtils.getConcreteItemId(aPlacesNode);
      if (concreteId != -1) {
        
        if (PlacesUtils.nodeIsQuery(aPlacesNode) ||
            (concreteId != aPlacesNode.itemId && !aResolveShortcuts)) {
          aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
          aJSNode.uri = aPlacesNode.uri;
          
          if (aIsUICommand)
            aJSNode.concreteId = concreteId;
        }
        else { 
          aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER;

          
          if (aJSNode.id == PlacesUtils.placesRootId)
            aJSNode.root = "placesRoot";
          else if (aJSNode.id == PlacesUtils.bookmarksMenuFolderId)
            aJSNode.root = "bookmarksMenuFolder";
          else if (aJSNode.id == PlacesUtils.tagsFolderId)
            aJSNode.root = "tagsFolder";
          else if (aJSNode.id == PlacesUtils.unfiledBookmarksFolderId)
            aJSNode.root = "unfiledBookmarksFolder";
          else if (aJSNode.id == PlacesUtils.toolbarFolderId)
            aJSNode.root = "toolbarFolder";
        }
      }
      else {
        
        aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
        aJSNode.uri = aPlacesNode.uri;
      }
    }

    function appendConvertedComplexNode(aNode, aSourceNode, aArray) {
      var repr = {};

      for (let [name, value] in Iterator(aNode))
        repr[name] = value;

      
      var children = repr.children = [];
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
          appendConvertedNode(aSourceNode.getChild(i), i, children);
        }
        if (!wasOpen)
          aSourceNode.containerOpen = false;
      }

      aArray.push(repr);
      return true;
    }

    function appendConvertedNode(bNode, aIndex, aArray) {
      var node = {};

      
      
      
      if (aIndex)
        node.index = aIndex;

      addGenericProperties(bNode, node);

      var parent = bNode.parent;
      var grandParent = parent ? parent.parent : null;

      if (PlacesUtils.nodeIsURI(bNode)) {
        
        if (parent && parent.itemId == PlacesUtils.tagsFolderId)
          return false;

        
        
        
        try {
          PlacesUtils._uri(bNode.uri);
        } catch (ex) {
          return false;
        }

        addURIProperties(bNode, node);
      }
      else if (PlacesUtils.nodeIsContainer(bNode)) {
        
        if (grandParent && grandParent.itemId == PlacesUtils.tagsFolderId)
          return false;

        addContainerProperties(bNode, node);
      }
      else if (PlacesUtils.nodeIsSeparator(bNode)) {
        
        
        if ((parent && parent.itemId == PlacesUtils.tagsFolderId) ||
            (grandParent && grandParent.itemId == PlacesUtils.tagsFolderId))
          return false;

        addSeparatorProperties(bNode, node);
      }

      if (!node.feedURI && node.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER)
        return appendConvertedComplexNode(node, bNode, aArray);

      aArray.push(node);
      return true;
    }

    
    var array = [];
    if (appendConvertedNode(aNode, null, array)) {
      var json = JSON.stringify(array[0]);
      aStream.write(json, json.length);
    }
    else {
      throw Cr.NS_ERROR_UNEXPECTED;
    }
  },

  

















  serializeNodeAsJSONToOutputStream:
  function PU_serializeNodeAsJSONToOutputStream(aNode, aStream, aIsUICommand,
                                                aResolveShortcuts,
                                                aExcludeItems) {
    Deprecated.warning(
      "serializeNodeAsJSONToOutputStream is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=854761");

    this._serializeNodeAsJSONToOutputStream(aNode, aStream, aIsUICommand,
                                            aResolveShortcuts, aExcludeItems);
  },

  







  restoreBookmarksFromJSONFile:
  function PU_restoreBookmarksFromJSONFile(aFile) {
    Deprecated.warning(
      "restoreBookmarksFromJSONFile is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=854388");

    BookmarkJSONUtils.importFromFile(aFile, true);
  },

  




  backupBookmarksToFile: function PU_backupBookmarksToFile(aFile) {
    Deprecated.warning(
      "backupBookmarksToFile is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=852041");
    return PlacesBackups.saveBookmarksToJSONFile(aFile);
  },

  





  archiveBookmarksFile:
  function PU_archiveBookmarksFile(aMaxBackups, aForceBackup) {
    Deprecated.warning(
      "archiveBookmarksFile is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=857429");
    return PlacesBackups.create(aMaxBackups, aForceBackup);
  },

  


  get backups() {
    Deprecated.warning(
      "PlacesUtils.backups is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=857429");
    return PlacesBackups;
  },

  
















  asyncGetBookmarkIds: function PU_asyncGetBookmarkIds(aURI, aCallback, aScope)
  {
    if (!this._asyncGetBookmarksStmt) {
      let db = this.history.DBConnection;
      this._asyncGetBookmarksStmt = db.createAsyncStatement(
        "SELECT b.id "
      + "FROM moz_bookmarks b "
      + "JOIN moz_places h on h.id = b.fk "
      + "WHERE h.url = :url "
      );
      this.registerShutdownFunction(function () {
        this._asyncGetBookmarksStmt.finalize();
      });
    }

    let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
    this._asyncGetBookmarksStmt.params.url = url;

    
    
    let stmt = new AsyncStatementCancelWrapper(this._asyncGetBookmarksStmt);
    return stmt.executeAsync({
      _itemIds: [],
      handleResult: function(aResultSet) {
        for (let row; (row = aResultSet.getNextRow());) {
          this._itemIds.push(row.getResultByIndex(0));
        }
      },
      handleCompletion: function(aReason)
      {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          aCallback.apply(aScope, [this._itemIds, aURI]);
        }
      }
    });
  },

  













  _bookmarksServiceReady: false,
  _bookmarksServiceObserversQueue: [],
  addLazyBookmarkObserver:
  function PU_addLazyBookmarkObserver(aObserver, aWeakOwner) {
    if (this._bookmarksServiceReady) {
      this.bookmarks.addObserver(aObserver, aWeakOwner === true);
      return;
    }
    this._bookmarksServiceObserversQueue.push({ observer: aObserver,
                                                weak: aWeakOwner === true });
  },

  





  removeLazyBookmarkObserver:
  function PU_removeLazyBookmarkObserver(aObserver) {
    if (this._bookmarksServiceReady) {
      this.bookmarks.removeObserver(aObserver);
      return;
    }
    let index = -1;
    for (let i = 0;
         i < this._bookmarksServiceObserversQueue.length && index == -1; i++) {
      if (this._bookmarksServiceObserversQueue[i].observer === aObserver)
        index = i;
    }
    if (index != -1) {
      this._bookmarksServiceObserversQueue.splice(index, 1);
    }
  },

  






  setCharsetForURI: function PU_setCharsetForURI(aURI, aCharset) {
    let deferred = Promise.defer();

    
    
    Services.tm.mainThread.dispatch(function() {
      if (aCharset && aCharset.length > 0) {
        PlacesUtils.annotations.setPageAnnotation(
          aURI, PlacesUtils.CHARSET_ANNO, aCharset, 0,
          Ci.nsIAnnotationService.EXPIRE_NEVER);
      } else {
        PlacesUtils.annotations.removePageAnnotation(
          aURI, PlacesUtils.CHARSET_ANNO);
      }
      deferred.resolve();
    }, Ci.nsIThread.DISPATCH_NORMAL);

    return deferred.promise;
  },

  






  getCharsetForURI: function PU_getCharsetForURI(aURI) {
    let deferred = Promise.defer();

    Services.tm.mainThread.dispatch(function() {
      let charset = null;

      try {
        charset = PlacesUtils.annotations.getPageAnnotation(aURI,
                                                            PlacesUtils.CHARSET_ANNO);
      } catch (ex) { }

      deferred.resolve(charset);
    }, Ci.nsIThread.DISPATCH_NORMAL);

    return deferred.promise;
  },

  






  promiseUpdatePlace: function PU_promiseUpdatePlaces(aPlace) {
    let deferred = Promise.defer();
    PlacesUtils.asyncHistory.updatePlaces(aPlace, {
      _placeInfo: null,
      handleResult: function handleResult(aPlaceInfo) {
        this._placeInfo = aPlaceInfo;
      },
      handleError: function handleError(aResultCode, aPlaceInfo) {
        deferred.reject(new Components.Exception("Error", aResultCode));
      },
      handleCompletion: function() {
        deferred.resolve(this._placeInfo);
      }
    });

    return deferred.promise;
  },

  






  promisePlaceInfo: function PU_promisePlaceInfo(aPlaceIdentifier) {
    let deferred = Promise.defer();
    PlacesUtils.asyncHistory.getPlacesInfo(aPlaceIdentifier, {
      _placeInfo: null,
      handleResult: function handleResult(aPlaceInfo) {
        this._placeInfo = aPlaceInfo;
      },
      handleError: function handleError(aResultCode, aPlaceInfo) {
        deferred.reject(new Components.Exception("Error", aResultCode));
      },
      handleCompletion: function() {
        deferred.resolve(this._placeInfo);
      }
    });

    return deferred.promise;
  }
};





function AsyncStatementCancelWrapper(aStmt) {
  this._stmt = aStmt;
}
AsyncStatementCancelWrapper.prototype = {
  _canceled: false,
  _cancel: function() {
    this._canceled = true;
    this._pendingStmt.cancel();
  },
  handleResult: function(aResultSet) {
    this._callback.handleResult(aResultSet);
  },
  handleError: function(aError) {
    Cu.reportError("Async statement execution returned (" + aError.result +
                   "): " + aError.message);
  },
  handleCompletion: function(aReason)
  {
    let reason = this._canceled ?
                   Ci.mozIStorageStatementCallback.REASON_CANCELED :
                   aReason;
    this._callback.handleCompletion(reason);
  },
  executeAsync: function(aCallback) {
    this._pendingStmt = this._stmt.executeAsync(this);
    this._callback = aCallback;
    let self = this;
    return { cancel: function () { self._cancel(); } }
  }
}

XPCOMUtils.defineLazyGetter(PlacesUtils, "history", function() {
  return Cc["@mozilla.org/browser/nav-history-service;1"]
           .getService(Ci.nsINavHistoryService)
           .QueryInterface(Ci.nsIBrowserHistory)
           .QueryInterface(Ci.nsPIPlacesDatabase);
});

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "asyncHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");

XPCOMUtils.defineLazyGetter(PlacesUtils, "bhistory", function() {
  return PlacesUtils.history;
});

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "favicons",
                                   "@mozilla.org/browser/favicon-service;1",
                                   "mozIAsyncFavicons");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "bookmarks",
                                   "@mozilla.org/browser/nav-bookmarks-service;1",
                                   "nsINavBookmarksService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "annotations",
                                   "@mozilla.org/browser/annotation-service;1",
                                   "nsIAnnotationService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "tagging",
                                   "@mozilla.org/browser/tagging-service;1",
                                   "nsITaggingService");

XPCOMUtils.defineLazyGetter(PlacesUtils, "livemarks", function() {
  return Cc["@mozilla.org/browser/livemark-service;2"].
         getService(Ci.mozIAsyncLivemarks);
});

XPCOMUtils.defineLazyGetter(PlacesUtils, "transactionManager", function() {
  let tm = Cc["@mozilla.org/transactionmanager;1"].
           createInstance(Ci.nsITransactionManager);
  tm.AddListener(PlacesUtils);
  this.registerShutdownFunction(function () {
    
    
    this.transactionManager.RemoveListener(this);
    this.transactionManager.clear();
  });

  
  
  
  
  
  
  
  
  
  
  
  
  return Object.create(tm, {
    "doTransaction": {
      value: function(aTransaction) {
        tm.doTransaction(aTransaction);
      }
    }
  });
});

XPCOMUtils.defineLazyGetter(this, "bundle", function() {
  const PLACES_STRING_BUNDLE_URI = "chrome://places/locale/places.properties";
  return Cc["@mozilla.org/intl/stringbundle;1"].
         getService(Ci.nsIStringBundleService).
         createBundle(PLACES_STRING_BUNDLE_URI);
});

XPCOMUtils.defineLazyServiceGetter(this, "focusManager",
                                   "@mozilla.org/focus-manager;1",
                                   "nsIFocusManager");








function updateCommandsOnActiveWindow()
{
  let win = focusManager.activeWindow;
  if (win && win instanceof Ci.nsIDOMWindow) {
    
    win.updateCommands("undo");
  }
}








function TransactionItemCache()
{
}

TransactionItemCache.prototype = {
  set id(v)
    this._id = (parseInt(v) > 0 ? v : null),
  get id()
    this._id || -1,
  set parentId(v)
    this._parentId = (parseInt(v) > 0 ? v : null),
  get parentId()
    this._parentId || -1,
  keyword: null,
  title: null,
  dateAdded: null,
  lastModified: null,
  postData: null,
  itemType: null,
  set uri(v)
    this._uri = (v instanceof Ci.nsIURI ? NetUtil.newURI(v.spec) : null),
  get uri()
    this._uri || null,
  set feedURI(v)
    this._feedURI = (v instanceof Ci.nsIURI ? NetUtil.newURI(v.spec) : null),
  get feedURI()
    this._feedURI || null,
  set siteURI(v)
    this._siteURI = (v instanceof Ci.nsIURI ? NetUtil.newURI(v.spec) : null),
  get siteURI()
    this._siteURI || null,
  set index(v)
    this._index = (parseInt(v) >= 0 ? v : null),
  
  get index()
    this._index != null ? this._index : PlacesUtils.bookmarks.DEFAULT_INDEX,
  set annotations(v)
    this._annotations = Array.isArray(v) ? JSON.parse(JSON.stringify(v)) : null,
  get annotations()
    this._annotations || null,
  set tags(v)
    this._tags = (v && Array.isArray(v) ? Array.slice(v) : null),
  get tags()
    this._tags || null,
};







function BaseTransaction()
{
}

BaseTransaction.prototype = {
  name: null,
  set childTransactions(v)
    this._childTransactions = (Array.isArray(v) ? Array.slice(v) : null),
  get childTransactions()
    this._childTransactions || null,
  doTransaction: function BTXN_doTransaction() {},
  redoTransaction: function BTXN_redoTransaction() this.doTransaction(),
  undoTransaction: function BTXN_undoTransaction() {},
  merge: function BTXN_merge() false,
  get isTransient() false,
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITransaction
  ]),
};












this.PlacesAggregatedTransaction =
 function PlacesAggregatedTransaction(aName, aTransactions)
{
  
  
  this.childTransactions = aTransactions;
  this.name = aName;
  this.item = new TransactionItemCache();

  
  
  let countTransactions = function(aTransactions, aTxnCount)
  {
    for (let i = 0;
         i < aTransactions.length && aTxnCount < MIN_TRANSACTIONS_FOR_BATCH;
         ++i, ++aTxnCount) {
      let txn = aTransactions[i];
      if (txn.childTransactions && txn.childTransactions.length > 0)
        aTxnCount = countTransactions(txn.childTransactions, aTxnCount);
    }
    return aTxnCount;
  }

  let txnCount = countTransactions(this.childTransactions, 0);
  this._useBatch = txnCount >= MIN_TRANSACTIONS_FOR_BATCH;
}

PlacesAggregatedTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function ATXN_doTransaction()
  {
    this._isUndo = false;
    if (this._useBatch)
      PlacesUtils.bookmarks.runInBatchMode(this, null);
    else
      this.runBatched(false);
  },

  undoTransaction: function ATXN_undoTransaction()
  {
    this._isUndo = true;
    if (this._useBatch)
      PlacesUtils.bookmarks.runInBatchMode(this, null);
    else
      this.runBatched(true);
  },

  runBatched: function ATXN_runBatched()
  {
    
    
    let transactions = this.childTransactions.slice(0);
    if (this._isUndo)
      transactions.reverse();
    for (let i = 0; i < transactions.length; ++i) {
      let txn = transactions[i];
      if (this.item.parentId != -1)
        txn.item.parentId = this.item.parentId;
      if (this._isUndo)
        txn.undoTransaction();
      else
        txn.doTransaction();
    }
  }
};


















this.PlacesCreateFolderTransaction =
 function PlacesCreateFolderTransaction(aTitle, aParentId, aIndex, aAnnotations,
                                        aChildTransactions)
{
  this.item = new TransactionItemCache();
  this.item.title = aTitle;
  this.item.parentId = aParentId;
  this.item.index = aIndex;
  this.item.annotations = aAnnotations;
  this.childTransactions = aChildTransactions;
}

PlacesCreateFolderTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function CFTXN_doTransaction()
  { 
    this.item.id = PlacesUtils.bookmarks.createFolder(this.item.parentId,
                                                      this.item.title,
                                                      this.item.index);
    if (this.item.annotations && this.item.annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this.item.id, this.item.annotations);

    if (this.childTransactions && this.childTransactions.length > 0) {
      
      for (let i = 0; i < this.childTransactions.length; ++i) {
        this.childTransactions[i].item.parentId = this.item.id;
      }

      let txn = new PlacesAggregatedTransaction("Create folder childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }
  },

  undoTransaction: function CFTXN_undoTransaction()
  {
    if (this.childTransactions && this.childTransactions.length > 0) {
      let txn = new PlacesAggregatedTransaction("Create folder childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }

    
    PlacesUtils.bookmarks.removeItem(this.item.id);
  }
};



























this.PlacesCreateBookmarkTransaction =
 function PlacesCreateBookmarkTransaction(aURI, aParentId, aIndex, aTitle,
                                          aKeyword, aAnnotations,
                                          aChildTransactions)
{
  this.item = new TransactionItemCache();
  this.item.uri = aURI;
  this.item.parentId = aParentId;
  this.item.index = aIndex;
  this.item.title = aTitle;
  this.item.keyword = aKeyword;
  this.item.annotations = aAnnotations;
  this.childTransactions = aChildTransactions;
}

PlacesCreateBookmarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function CITXN_doTransaction()
  {
    this.item.id = PlacesUtils.bookmarks.insertBookmark(this.item.parentId,
                                                        this.item.uri,
                                                        this.item.index,
                                                        this.item.title);
    if (this.item.keyword) {
      PlacesUtils.bookmarks.setKeywordForBookmark(this.item.id,
                                                  this.item.keyword);
    }
    if (this.item.annotations && this.item.annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this.item.id, this.item.annotations);
 
    if (this.childTransactions && this.childTransactions.length > 0) {
      
      for (let i = 0; i < this.childTransactions.length; ++i) {
        this.childTransactions[i].item.id = this.item.id;
      }
      let txn = new PlacesAggregatedTransaction("Create item childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }
  },

  undoTransaction: function CITXN_undoTransaction()
  {
    if (this.childTransactions && this.childTransactions.length > 0) {
      
      let txn = new PlacesAggregatedTransaction("Create item childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }

    
    PlacesUtils.bookmarks.removeItem(this.item.id);
  }
};












this.PlacesCreateSeparatorTransaction =
 function PlacesCreateSeparatorTransaction(aParentId, aIndex)
{
  this.item = new TransactionItemCache();
  this.item.parentId = aParentId;
  this.item.index = aIndex;
}

PlacesCreateSeparatorTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function CSTXN_doTransaction()
  {
    this.item.id =
      PlacesUtils.bookmarks.insertSeparator(this.item.parentId, this.item.index);
  },

  undoTransaction: function CSTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.removeItem(this.item.id);
  }
};






















this.PlacesCreateLivemarkTransaction =
 function PlacesCreateLivemarkTransaction(aFeedURI, aSiteURI, aTitle, aParentId,
                                          aIndex, aAnnotations)
{
  this.item = new TransactionItemCache();
  this.item.feedURI = aFeedURI;
  this.item.siteURI = aSiteURI;
  this.item.title = aTitle;
  this.item.parentId = aParentId;
  this.item.index = aIndex;
  this.item.annotations = aAnnotations;
}

PlacesCreateLivemarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function CLTXN_doTransaction()
  {
    PlacesUtils.livemarks.addLivemark(
      { title: this.item.title
      , feedURI: this.item.feedURI
      , parentId: this.item.parentId
      , index: this.item.index
      , siteURI: this.item.siteURI
      },
      (function(aStatus, aLivemark) {
        if (Components.isSuccessCode(aStatus)) {
          this.item.id = aLivemark.id;
          if (this.item.annotations && this.item.annotations.length > 0) {
            PlacesUtils.setAnnotationsForItem(this.item.id,
                                              this.item.annotations);
          }
        }
      }).bind(this)
    );
  },

  undoTransaction: function CLTXN_undoTransaction()
  {
    
    
    PlacesUtils.livemarks.getLivemark(
      { id: this.item.id },
      (function (aStatus, aLivemark) {
        PlacesUtils.bookmarks.removeItem(this.item.id);
      }).bind(this)
    );
  }
};











function PlacesRemoveLivemarkTransaction(aLivemarkId)
{
  this.item = new TransactionItemCache();
  this.item.id = aLivemarkId;
  this.item.title = PlacesUtils.bookmarks.getItemTitle(this.item.id);
  this.item.parentId = PlacesUtils.bookmarks.getFolderIdForItem(this.item.id);

  let annos = PlacesUtils.getAnnotationsForItem(this.item.id);
  
  let annosToExclude = [PlacesUtils.LMANNO_FEEDURI,
                        PlacesUtils.LMANNO_SITEURI];
  this.item.annotations = annos.filter(function(aValue, aIndex, aArray) {
      return annosToExclude.indexOf(aValue.name) == -1;
    });
  this.item.dateAdded = PlacesUtils.bookmarks.getItemDateAdded(this.item.id);
  this.item.lastModified =
    PlacesUtils.bookmarks.getItemLastModified(this.item.id);
}

PlacesRemoveLivemarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function RLTXN_doTransaction()
  {
    PlacesUtils.livemarks.getLivemark(
      { id: this.item.id },
      (function (aStatus, aLivemark) {
        if (Components.isSuccessCode(aStatus)) {
          this.item.feedURI = aLivemark.feedURI;
          this.item.siteURI = aLivemark.siteURI;

          PlacesUtils.bookmarks.removeItem(this.item.id);
        }
      }).bind(this)
    );
  },

  undoTransaction: function RLTXN_undoTransaction()
  {
    
    
    
    
    PlacesUtils.livemarks.getLivemark(
      { id: this.item.id },
      (function () {
        let addLivemarkCallback = (function(aStatus, aLivemark) {
          if (Components.isSuccessCode(aStatus)) {
            let itemId = aLivemark.id;
            PlacesUtils.bookmarks.setItemDateAdded(itemId, this.item.dateAdded);
            PlacesUtils.setAnnotationsForItem(itemId, this.item.annotations);
          }
        }).bind(this);
        PlacesUtils.livemarks.addLivemark({ parentId: this.item.parentId
                                          , title: this.item.title
                                          , siteURI: this.item.siteURI
                                          , feedURI: this.item.feedURI
                                          , index: this.item.index
                                          , lastModified: this.item.lastModified
                                          },
                                          addLivemarkCallback);
      }).bind(this)
    );
  }
};














this.PlacesMoveItemTransaction =
 function PlacesMoveItemTransaction(aItemId, aNewParentId, aNewIndex)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.item.parentId = PlacesUtils.bookmarks.getFolderIdForItem(this.item.id);
  this.new = new TransactionItemCache();
  this.new.parentId = aNewParentId;
  this.new.index = aNewIndex;
}

PlacesMoveItemTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function MITXN_doTransaction()
  {
    this.item.index = PlacesUtils.bookmarks.getItemIndex(this.item.id);
    PlacesUtils.bookmarks.moveItem(this.item.id,
                                   this.new.parentId, this.new.index);
    this._undoIndex = PlacesUtils.bookmarks.getItemIndex(this.item.id);
  },

  undoTransaction: function MITXN_undoTransaction()
  {
    
    
    if (this.new.parentId == this.item.parentId &&
        this.item.index > this._undoIndex) {
      PlacesUtils.bookmarks.moveItem(this.item.id, this.item.parentId,
                                     this.item.index + 1);
    }
    else {
      PlacesUtils.bookmarks.moveItem(this.item.id, this.item.parentId,
                                     this.item.index);
    }
  }
};










this.PlacesRemoveItemTransaction =
 function PlacesRemoveItemTransaction(aItemId)
{
  if (PlacesUtils.isRootItem(aItemId))
    throw Cr.NS_ERROR_INVALID_ARG;

  
  let parent = PlacesUtils.bookmarks.getFolderIdForItem(aItemId);
  let grandparent = PlacesUtils.bookmarks.getFolderIdForItem(parent);
  if (grandparent == PlacesUtils.tagsFolderId) {
    let uri = PlacesUtils.bookmarks.getBookmarkURI(aItemId);
    return new PlacesUntagURITransaction(uri, [parent]);
  }

  
  if (PlacesUtils.annotations.itemHasAnnotation(aItemId,
                                                PlacesUtils.LMANNO_FEEDURI))
    return new PlacesRemoveLivemarkTransaction(aItemId);

  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.item.itemType = PlacesUtils.bookmarks.getItemType(this.item.id);
  if (this.item.itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
    this.childTransactions = this._getFolderContentsTransactions();
    
    let txn = PlacesUtils.bookmarks.getRemoveFolderTransaction(this.item.id);
    this.childTransactions.push(txn);
  }
  else if (this.item.itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
    this.item.uri = PlacesUtils.bookmarks.getBookmarkURI(this.item.id);
    this.item.keyword =
      PlacesUtils.bookmarks.getKeywordForBookmark(this.item.id);
  }

  if (this.item.itemType != Ci.nsINavBookmarksService.TYPE_SEPARATOR)
    this.item.title = PlacesUtils.bookmarks.getItemTitle(this.item.id);

  this.item.parentId = PlacesUtils.bookmarks.getFolderIdForItem(this.item.id);
  this.item.annotations = PlacesUtils.getAnnotationsForItem(this.item.id);
  this.item.dateAdded = PlacesUtils.bookmarks.getItemDateAdded(this.item.id);
  this.item.lastModified =
    PlacesUtils.bookmarks.getItemLastModified(this.item.id);
}

PlacesRemoveItemTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function RITXN_doTransaction()
  {
    this.item.index = PlacesUtils.bookmarks.getItemIndex(this.item.id);

    if (this.item.itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      let txn = new PlacesAggregatedTransaction("Remove item childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }
    else {
      
      let tags = this.item.uri ?
        PlacesUtils.tagging.getTagsForURI(this.item.uri) : null;

      PlacesUtils.bookmarks.removeItem(this.item.id);

      
      
      if (tags && PlacesUtils.getMostRecentBookmarkForURI(this.item.uri) == -1) {
        this.item.tags = tags;
      }
    }
  },

  undoTransaction: function RITXN_undoTransaction()
  {
    if (this.item.itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
      this.item.id = PlacesUtils.bookmarks.insertBookmark(this.item.parentId,
                                                          this.item.uri,
                                                          this.item.index,
                                                          this.item.title);
      if (this.item.tags && this.item.tags.length > 0)
        PlacesUtils.tagging.tagURI(this.item.uri, this.item.tags);
      if (this.item.keyword) {
        PlacesUtils.bookmarks.setKeywordForBookmark(this.item.id,
                                                    this.item.keyword);
      }
    }
    else if (this.item.itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      let txn = new PlacesAggregatedTransaction("Remove item childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }
    else { 
      this.item.id = PlacesUtils.bookmarks.insertSeparator(this.item.parentId,
                                                            this.item.index);
    }

    if (this.item.annotations && this.item.annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this.item.id, this.item.annotations);

    PlacesUtils.bookmarks.setItemDateAdded(this.item.id, this.item.dateAdded);
    PlacesUtils.bookmarks.setItemLastModified(this.item.id,
                                              this.item.lastModified);
  },

  



  _getFolderContentsTransactions:
  function RITXN__getFolderContentsTransactions()
  {
    let transactions = [];
    let contents =
      PlacesUtils.getFolderContents(this.item.id, false, false).root;
    for (let i = 0; i < contents.childCount; ++i) {
      let txn = new PlacesRemoveItemTransaction(contents.getChild(i).itemId);
      transactions.push(txn);
    }
    contents.containerOpen = false;
    
    return transactions.reverse();
  }
};












this.PlacesEditItemTitleTransaction =
 function PlacesEditItemTitleTransaction(aItemId, aNewTitle)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.title = aNewTitle;
}

PlacesEditItemTitleTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EITTXN_doTransaction()
  {
    this.item.title = PlacesUtils.bookmarks.getItemTitle(this.item.id);
    PlacesUtils.bookmarks.setItemTitle(this.item.id, this.new.title);
  },

  undoTransaction: function EITTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemTitle(this.item.id, this.item.title);
  }
};












this.PlacesEditBookmarkURITransaction =
 function PlacesEditBookmarkURITransaction(aItemId, aNewURI) {
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.uri = aNewURI;
}

PlacesEditBookmarkURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBUTXN_doTransaction()
  {
    this.item.uri = PlacesUtils.bookmarks.getBookmarkURI(this.item.id);
    PlacesUtils.bookmarks.changeBookmarkURI(this.item.id, this.new.uri);
    
    this.item.tags = PlacesUtils.tagging.getTagsForURI(this.item.uri);
    if (this.item.tags.length != 0) {
      
      if (PlacesUtils.getBookmarksForURI(this.item.uri, {}).length == 0)
        PlacesUtils.tagging.untagURI(this.item.uri, this.item.tags);
      PlacesUtils.tagging.tagURI(this.new.URI, this.item.tags);
    }
  },

  undoTransaction: function EBUTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.changeBookmarkURI(this.item.id, this.item.uri);
    
    if (this.item.tags.length != 0) {
      
      if (PlacesUtils.getBookmarksForURI(this.new.uri, {}).length == 0)
        PlacesUtils.tagging.untagURI(this.new.uri, this.item.tags);
      PlacesUtils.tagging.tagURI(this.item.uri, this.item.tags);
    }
  }
};















this.PlacesSetItemAnnotationTransaction =
 function PlacesSetItemAnnotationTransaction(aItemId, aAnnotationObject)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.annotations = [aAnnotationObject];
}

PlacesSetItemAnnotationTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SIATXN_doTransaction()
  {
    let annoName = this.new.annotations[0].name;
    if (PlacesUtils.annotations.itemHasAnnotation(this.item.id, annoName)) {
      
      let flags = {}, expires = {}, mimeType = {}, type = {};
      PlacesUtils.annotations.getItemAnnotationInfo(this.item.id, annoName, flags,
                                                    expires, mimeType, type);
      let value = PlacesUtils.annotations.getItemAnnotation(this.item.id,
                                                            annoName);
      this.item.annotations = [{ name: annoName,
                                type: type.value,
                                flags: flags.value,
                                value: value,
                                expires: expires.value }];
    }
    else {
      
      this.item.annotations = [{ name: annoName,
                                type: Ci.nsIAnnotationService.TYPE_STRING,
                                flags: 0,
                                value: null,
                                expires: Ci.nsIAnnotationService.EXPIRE_NEVER }];
    }

    PlacesUtils.setAnnotationsForItem(this.item.id, this.new.annotations);
  },

  undoTransaction: function SIATXN_undoTransaction()
  {
    PlacesUtils.setAnnotationsForItem(this.item.id, this.item.annotations);
  }
};















this.PlacesSetPageAnnotationTransaction =
 function PlacesSetPageAnnotationTransaction(aURI, aAnnotationObject)
{
  this.item = new TransactionItemCache();
  this.item.uri = aURI;
  this.new = new TransactionItemCache();
  this.new.annotations = [aAnnotationObject];
}

PlacesSetPageAnnotationTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SPATXN_doTransaction()
  {
    let annoName = this.new.annotations[0].name;
    if (PlacesUtils.annotations.pageHasAnnotation(this.item.uri, annoName)) {
      
      let flags = {}, expires = {}, mimeType = {}, type = {};
      PlacesUtils.annotations.getPageAnnotationInfo(this.item.uri, annoName, flags,
                                                    expires, mimeType, type);
      let value = PlacesUtils.annotations.getPageAnnotation(this.item.uri,
                                                            annoName);
      this.item.annotations = [{ name: annoName,
                                type: type.value,
                                flags: flags.value,
                                value: value,
                                expires: expires.value }];
    }
    else {
      
      this.item.annotations = [{ name: annoName,
                                type: Ci.nsIAnnotationService.TYPE_STRING,
                                flags: 0,
                                value: null,
                                expires: Ci.nsIAnnotationService.EXPIRE_NEVER }];
    }

    PlacesUtils.setAnnotationsForURI(this.item.uri, this.new.annotations);
  },

  undoTransaction: function SPATXN_undoTransaction()
  {
    PlacesUtils.setAnnotationsForURI(this.item.uri, this.item.annotations);
  }
};












this.PlacesEditBookmarkKeywordTransaction =
 function PlacesEditBookmarkKeywordTransaction(aItemId, aNewKeyword)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.keyword = aNewKeyword;
}

PlacesEditBookmarkKeywordTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBKTXN_doTransaction()
  {
    this.item.keyword = PlacesUtils.bookmarks.getKeywordForBookmark(this.item.id);
    PlacesUtils.bookmarks.setKeywordForBookmark(this.item.id, this.new.keyword);
  },

  undoTransaction: function EBKTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setKeywordForBookmark(this.item.id, this.item.keyword);
  }
};












this.PlacesEditBookmarkPostDataTransaction =
 function PlacesEditBookmarkPostDataTransaction(aItemId, aPostData)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.postData = aPostData;
}

PlacesEditBookmarkPostDataTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBPDTXN_doTransaction()
  {
    this.item.postData = PlacesUtils.getPostDataForBookmark(this.item.id);
    PlacesUtils.setPostDataForBookmark(this.item.id, this.new.postData);
  },

  undoTransaction: function EBPDTXN_undoTransaction()
  {
    PlacesUtils.setPostDataForBookmark(this.item.id, this.item.postData);
  }
};












this.PlacesEditItemDateAddedTransaction =
 function PlacesEditItemDateAddedTransaction(aItemId, aNewDateAdded)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.dateAdded = aNewDateAdded;
}

PlacesEditItemDateAddedTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EIDATXN_doTransaction()
  {
    
    if (this.item.id == -1 && this.item.parentId != -1)
      this.item.id = this.item.parentId;
    this.item.dateAdded =
      PlacesUtils.bookmarks.getItemDateAdded(this.item.id);
    PlacesUtils.bookmarks.setItemDateAdded(this.item.id, this.new.dateAdded);
  },

  undoTransaction: function EIDATXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemDateAdded(this.item.id, this.item.dateAdded);
  }
};












this.PlacesEditItemLastModifiedTransaction =
 function PlacesEditItemLastModifiedTransaction(aItemId, aNewLastModified)
{
  this.item = new TransactionItemCache();
  this.item.id = aItemId;
  this.new = new TransactionItemCache();
  this.new.lastModified = aNewLastModified;
}

PlacesEditItemLastModifiedTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction:
  function EILMTXN_doTransaction()
  {
    
    if (this.item.id == -1 && this.item.parentId != -1)
      this.item.id = this.item.parentId;
    this.item.lastModified =
      PlacesUtils.bookmarks.getItemLastModified(this.item.id);
    PlacesUtils.bookmarks.setItemLastModified(this.item.id,
                                              this.new.lastModified);
  },

  undoTransaction:
  function EILMTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemLastModified(this.item.id,
                                              this.item.lastModified);
  }
};










this.PlacesSortFolderByNameTransaction =
 function PlacesSortFolderByNameTransaction(aFolderId)
{
  this.item = new TransactionItemCache();  
  this.item.id = aFolderId;
}

PlacesSortFolderByNameTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SFBNTXN_doTransaction()
  {
    this._oldOrder = [];

    let contents =
      PlacesUtils.getFolderContents(this.item.id, false, false).root;
    let count = contents.childCount;

    
    let newOrder = []; 
    let preSep = []; 
    let sortingMethod =
      function (a, b) {
        if (PlacesUtils.nodeIsContainer(a) && !PlacesUtils.nodeIsContainer(b))
          return -1;
        if (!PlacesUtils.nodeIsContainer(a) && PlacesUtils.nodeIsContainer(b))
          return 1;
        return a.title.localeCompare(b.title);
      };

    for (let i = 0; i < count; ++i) {
      let item = contents.getChild(i);
      this._oldOrder[item.itemId] = i;
      if (PlacesUtils.nodeIsSeparator(item)) {
        if (preSep.length > 0) {
          preSep.sort(sortingMethod);
          newOrder = newOrder.concat(preSep);
          preSep.splice(0, preSep.length);
        }
        newOrder.push(item);
      }
      else
        preSep.push(item);
    }
    contents.containerOpen = false;

    if (preSep.length > 0) {
      preSep.sort(sortingMethod);
      newOrder = newOrder.concat(preSep);
    }

    
    let callback = {
      runBatched: function() {
        for (let i = 0; i < newOrder.length; ++i) {
          PlacesUtils.bookmarks.setItemIndex(newOrder[i].itemId, i);
        }
      }
    };
    PlacesUtils.bookmarks.runInBatchMode(callback, null);
  },

  undoTransaction: function SFBNTXN_undoTransaction()
  {
    let callback = {
      _self: this,
      runBatched: function() {
        for (item in this._self._oldOrder)
          PlacesUtils.bookmarks.setItemIndex(item, this._self._oldOrder[item]);
      }
    };
    PlacesUtils.bookmarks.runInBatchMode(callback, null);
  }
};













this.PlacesTagURITransaction =
 function PlacesTagURITransaction(aURI, aTags)
{
  this.item = new TransactionItemCache();
  this.item.uri = aURI;
  this.item.tags = aTags;
}

PlacesTagURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function TUTXN_doTransaction()
  {
    if (PlacesUtils.getMostRecentBookmarkForURI(this.item.uri) == -1) {
      
      
      this.item.id =
        PlacesUtils.bookmarks
                   .insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                   this.item.uri,
                                   PlacesUtils.bookmarks.DEFAULT_INDEX,
                                   PlacesUtils.history.getPageTitle(this.item.uri));
    }
    PlacesUtils.tagging.tagURI(this.item.uri, this.item.tags);
  },

  undoTransaction: function TUTXN_undoTransaction()
  {
    if (this.item.id != -1) {
      PlacesUtils.bookmarks.removeItem(this.item.id);
      this.item.id = -1;
    }
    PlacesUtils.tagging.untagURI(this.item.uri, this.item.tags);
  }
};













this.PlacesUntagURITransaction =
 function PlacesUntagURITransaction(aURI, aTags)
{
  this.item = new TransactionItemCache();
  this.item.uri = aURI;
  if (aTags) {
    
    
    
    let tags = [];
    for (let i = 0; i < aTags.length; ++i) {
      if (typeof(aTags[i]) == "number")
        tags.push(PlacesUtils.bookmarks.getItemTitle(aTags[i]));
      else
        tags.push(aTags[i]);
    }
    this.item.tags = tags;
  }
}

PlacesUntagURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function UTUTXN_doTransaction()
  {
    
    
    let tags = PlacesUtils.tagging.getTagsForURI(this.item.uri);
    this.item.tags = this.item.tags.filter(function (aTag) {
      return tags.indexOf(aTag) != -1;
    });
    PlacesUtils.tagging.untagURI(this.item.uri, this.item.tags);
  },

  undoTransaction: function UTUTXN_undoTransaction()
  {
    PlacesUtils.tagging.tagURI(this.item.uri, this.item.tags);
  }
};
