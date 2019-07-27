




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

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.importGlobalProperties(["URL"]);

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
                                  "resource://gre/modules/Deprecated.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Bookmarks",
                                  "resource://gre/modules/Bookmarks.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "History",
                                  "resource://gre/modules/History.jsm");




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
function asContainer(aNode) QI_node(aNode, Ci.nsINavHistoryContainerResultNode);
function asQuery(aNode) QI_node(aNode, Ci.nsINavHistoryQueryResultNode);











function notify(observers, notification, args) {
  for (let observer of observers) {
    try {
      observer[notification](...args);
    } catch (ex) {}
  }
}









function* notifyKeywordChange(url, keyword) {
  
  let bookmarks = [];
  yield PlacesUtils.bookmarks.fetch({ url }, b => bookmarks.push(b));
  
  for (let bookmark of bookmarks) {
    bookmark.id = yield PlacesUtils.promiseItemId(bookmark.guid);
    bookmark.parentId = yield PlacesUtils.promiseItemId(bookmark.parentGuid);
  }
  let observers = PlacesUtils.bookmarks.getObservers();
  gIgnoreKeywordNotifications = true;
  for (let bookmark of bookmarks) {
    notify(observers, "onItemChanged", [ bookmark.id, "keyword", false,
                                         keyword,
                                         bookmark.lastModified * 1000,
                                         bookmark.type,
                                         bookmark.parentId,
                                         bookmark.guid, bookmark.parentGuid
                                       ]);
  }
  gIgnoreKeywordNotifications = false;
}

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

  





  nodeIsURI: function PU_nodeIsURI(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI;
  },

  





  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  




  nodeAncestors: function* PU_nodeAncestors(aNode) {
    let node = aNode.parent;
    while (node) {
      yield node;
      node = node.parent;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver
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

        
        
        
        gKeywordsCachePromise.catch(Cu.reportError);
        break;
    }
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

  








  getReversedHost(url)
    url.host.split("").reverse().join("") + ".",

  












  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI) {
    
    
    
    
    
    function convertNode(cNode) {
      if (PlacesUtils.nodeIsFolder(cNode) &&
          cNode.type != Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT &&
          asQuery(cNode).queryOptions.excludeItems) {
        return [PlacesUtils.getFolderContents(cNode.itemId, false, true).root, true];
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
        this._serializeNodeAsJSONToOutputStream(node, writer);
        if (shouldClose)
          node.containerOpen = false;

        return writer.value;
      }
      case this.TYPE_X_MOZ_URL: {
        let gatherDataUrl = function (bNode) {
          if (isLivemark(bNode)) {
            return gatherLivemarkUrl(bNode) + NEWLINE + bNode.title;
          }

          if (PlacesUtils.nodeIsURI(bNode))
            return (aOverrideURI || bNode.uri) + NEWLINE + bNode.title;
          
          return "";
        };

        let [node, shouldClose] = convertNode(aNode);
        let dataUrl = gatherDataUrl(node);
        if (shouldClose)
          node.containerOpen = false;

        return dataUrl;
      }
      case this.TYPE_HTML: {
        let gatherDataHtml = function (bNode) {
          let htmlEscape = function (s) {
            s = s.replace(/&/g, "&amp;");
            s = s.replace(/>/g, "&gt;");
            s = s.replace(/</g, "&lt;");
            s = s.replace(/"/g, "&quot;");
            s = s.replace(/'/g, "&apos;");
            return s;
          };
          
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
      var flags = {}, exp = {}, storageType = {};
      annosvc.getPageAnnotationInfo(aURI, annoNames[i], flags, exp, storageType);
      val = annosvc.getPageAnnotation(aURI, annoNames[i]);
      annos.push({name: annoNames[i],
                  flags: flags.value,
                  expires: exp.value,
                  value: val});
    }
    return annos;
  },

  








  getAnnotationsForItem: function PU_getAnnotationsForItem(aItemId) {
    var annosvc = this.annotations;
    var annos = [], val = null;
    var annoNames = annosvc.getItemAnnotationNames(aItemId);
    for (var i = 0; i < annoNames.length; i++) {
      var flags = {}, exp = {}, storageType = {};
      annosvc.getItemAnnotationInfo(aItemId, annoNames[i], flags, exp, storageType);
      val = annosvc.getItemAnnotation(aItemId, annoNames[i]);
      annos.push({name: annoNames[i],
                  flags: flags.value,
                  expires: exp.value,
                  value: val});
    }
    return annos;
  },

  








  setAnnotationsForURI: function PU_setAnnotationsForURI(aURI, aAnnos) {
    var annosvc = this.annotations;
    aAnnos.forEach(function(anno) {
      if (anno.value === undefined || anno.value === null) {
        annosvc.removePageAnnotation(aURI, anno.name);
      }
      else {
        let flags = ("flags" in anno) ? anno.flags : 0;
        let expires = ("expires" in anno) ?
          anno.expires : Ci.nsIAnnotationService.EXPIRE_NEVER;
        annosvc.setPageAnnotation(aURI, anno.name, anno.value, flags, expires);
      }
    });
  },

  








  setAnnotationsForItem: function PU_setAnnotationsForItem(aItemId, aAnnos) {
    var annosvc = this.annotations;

    aAnnos.forEach(function(anno) {
      if (anno.value === undefined || anno.value === null) {
        annosvc.removeItemAnnotation(aItemId, anno.name);
      }
      else {
        let flags = ("flags" in anno) ? anno.flags : 0;
        let expires = ("expires" in anno) ?
          anno.expires : Ci.nsIAnnotationService.EXPIRE_NEVER;
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

  




  setPostDataForBookmark(aBookmarkId, aPostData) {
    if (!aPostData)
      throw new Error("Must provide valid POST data");
    
    
    
    let stmt = PlacesUtils.history.DBConnection.createStatement(
      `UPDATE moz_keywords SET post_data = :post_data
       WHERE id = (SELECT k.id FROM moz_keywords k
                   JOIN moz_bookmarks b ON b.fk = k.place_id
                   WHERE b.id = :item_id
                   AND post_data ISNULL
                   LIMIT 1)`);
    stmt.params.item_id = aBookmarkId;
    stmt.params.post_data = aPostData;
    try {
      stmt.execute();
    }
    finally {
      stmt.finalize();
    }

    
    return Task.spawn(function* () {
      let guid = yield PlacesUtils.promiseItemGuid(aBookmarkId);
      let bm = yield PlacesUtils.bookmarks.fetch(guid);

      
      let cache = yield gKeywordsCachePromise;
      for (let [ keyword, entry ] of cache) {
        
        if (entry.url.href == bm.url.href && !entry.postData) {
          entry.postData = aPostData;
        }
      }
    }).catch(Cu.reportError);
  },

  




  getPostDataForBookmark(aBookmarkId) {
    let stmt = PlacesUtils.history.DBConnection.createStatement(
      `SELECT k.post_data
       FROM moz_keywords k
       JOIN moz_places h ON h.id = k.place_id
       JOIN moz_bookmarks b ON b.fk = h.id
       WHERE b.id = :item_id`);
    stmt.params.item_id = aBookmarkId;
    try {
      if (!stmt.executeStep())
        return null;
      return stmt.row.post_data;
    }
    finally {
      stmt.finalize();
    }
  },

  






  getURLAndPostDataForKeyword(aKeyword) {
    Deprecated.warning("getURLAndPostDataForKeyword() is deprecated, please " +
                       "use PlacesUtils.keywords.fetch() instead",
                       "https://bugzilla.mozilla.org/show_bug.cgi?id=1100294");

    let stmt = PlacesUtils.history.DBConnection.createStatement(
      `SELECT h.url, k.post_data
       FROM moz_keywords k
       JOIN moz_places h ON h.id = k.place_id
       WHERE k.keyword = :keyword`);
    stmt.params.keyword = aKeyword.toLowerCase();
    try {
      if (!stmt.executeStep())
        return [ null, null ];
      return [ stmt.row.url, stmt.row.post_data ];
    }
    finally {
      stmt.finalize();
    }
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

  










  _serializeNodeAsJSONToOutputStream: function (aNode, aStream) {
    function addGenericProperties(aPlacesNode, aJSNode) {
      aJSNode.title = aPlacesNode.title;
      aJSNode.id = aPlacesNode.itemId;
      let guid = aPlacesNode.bookmarkGuid;
      if (guid) {
        aJSNode.itemGuid = guid;
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

      if (aPlacesNode.tags)
        aJSNode.tags = aPlacesNode.tags;

      
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
            concreteId != aPlacesNode.itemId) {
          aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
          aJSNode.uri = aPlacesNode.uri;
          
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
      if (grandParent)
        node.grandParentId = grandParent.itemId;

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

  






  promiseDBConnection: () => gAsyncDBConnPromised,

  





  promiseWrappedConnection: () => gAsyncDBWrapperPromised,

  














  asyncGetBookmarkIds: function PU_asyncGetBookmarkIds(aURI, aCallback)
  {
    let abort = false;
    let itemIds = [];
    Task.spawn(function* () {
      let conn = yield this.promiseDBConnection();
      const QUERY_STR = `SELECT b.id FROM moz_bookmarks b
                         JOIN moz_places h on h.id = b.fk
                         WHERE h.url = :url`;
      let spec = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
      yield conn.executeCached(QUERY_STR, { url: spec }, aRow => {
        if (abort)
          throw StopIteration;
        itemIds.push(aRow.getResultByIndex(0));  
      });
      if (!abort)
        aCallback(itemIds, aURI);
    }.bind(this)).then(null, Cu.reportError);
    return { cancel: () => { abort = true; } };
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
  },

  







  promiseFaviconData: function (aPageUrl) {
    let deferred = Promise.defer();
    PlacesUtils.favicons.getFaviconDataForPage(NetUtil.newURI(aPageUrl),
      function (aURI, aDataLen, aData, aMimeType) {
        if (aURI) {
          deferred.resolve({ uri: aURI,
                             dataLen: aDataLen,
                             data: aData,
                             mimeType: aMimeType });
        } else {
          deferred.reject();
        }
      });
    return deferred.promise;
  },

  






  promiseFaviconLinkUrl: function (aPageUrl) {
    let deferred = Promise.defer();
    if (!(aPageUrl instanceof Ci.nsIURI))
      aPageUrl = NetUtil.newURI(aPageUrl);

    PlacesUtils.favicons.getFaviconURLForPage(aPageUrl, uri => {
      if (uri) {
        uri = PlacesUtils.favicons.getFaviconLinkForIcon(uri);
        deferred.resolve(uri);
      } else {
        deferred.reject("favicon not found for uri");
      }
    });
    return deferred.promise;
  },

  


















  getImageURLForResolution:
  function PU_getImageURLForResolution(aWindow, aURL, aWidth = 16, aHeight = 16) {
    if (!aURL.endsWith('.ico') && !aURL.endsWith('.ICO')) {
      return aURL;
    }
    let width  = Math.round(aWidth * aWindow.devicePixelRatio);
    let height = Math.round(aHeight * aWindow.devicePixelRatio);
    return aURL + (aURL.contains("#") ? "&" : "#") +
           "-moz-resolution=" + width + "," + height;
  },

  









  promiseItemGuid: function (aItemId) GuidHelper.getItemGuid(aItemId),

  









  promiseItemId: function (aGuid) GuidHelper.getItemId(aGuid),

  


































































  promiseBookmarksTree: Task.async(function* (aItemGuid = "", aOptions = {}) {
    let createItemInfoObject = (aRow, aIncludeParentGuid) => {
      let item = {};
      let copyProps = (...props) => {
        for (let prop of props) {
          let val = aRow.getResultByName(prop);
          if (val !== null)
            item[prop] = val;
        }
      };
      copyProps("guid", "title", "index", "dateAdded", "lastModified");
      if (aIncludeParentGuid)
        copyProps("parentGuid");

      let itemId = aRow.getResultByName("id");
      if (aOptions.includeItemIds)
        item.id = itemId;

      
      GuidHelper.idsForGuids.set(item.guid, itemId);

      let type = aRow.getResultByName("type");
      if (type == Ci.nsINavBookmarksService.TYPE_BOOKMARK)
        copyProps("charset", "tags", "iconuri");

      
      if (aRow.getResultByName("has_annos")) {
        try {
          item.annos = PlacesUtils.getAnnotationsForItem(itemId);
        } catch (e) {
          Cu.reportError("Unexpected error while reading annotations " + e);
        }
      }

      switch (type) {
        case Ci.nsINavBookmarksService.TYPE_BOOKMARK:
          item.type = PlacesUtils.TYPE_X_MOZ_PLACE;
          
          item.uri = NetUtil.newURI(aRow.getResultByName("url")).spec;
          
          let keyword = PlacesUtils.bookmarks.getKeywordForBookmark(itemId);
          if (keyword)
            item.keyword = keyword;
          break;
        case Ci.nsINavBookmarksService.TYPE_FOLDER:
          item.type = PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER;
          
          if (itemId == PlacesUtils.placesRootId)
            item.root = "placesRoot";
          else if (itemId == PlacesUtils.bookmarksMenuFolderId)
            item.root = "bookmarksMenuFolder";
          else if (itemId == PlacesUtils.unfiledBookmarksFolderId)
            item.root = "unfiledBookmarksFolder";
          else if (itemId == PlacesUtils.toolbarFolderId)
            item.root = "toolbarFolder";
          break;
        case Ci.nsINavBookmarksService.TYPE_SEPARATOR:
          item.type = PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR;
          break;
        default:
          Cu.reportError("Unexpected bookmark type");
          break;
      }
      return item;
    };

    const QUERY_STR =
      `WITH RECURSIVE
       descendants(fk, level, type, id, guid, parent, parentGuid, position,
                   title, dateAdded, lastModified) AS (
         SELECT b1.fk, 0, b1.type, b1.id, b1.guid, b1.parent,
                (SELECT guid FROM moz_bookmarks WHERE id = b1.parent),
                b1.position, b1.title, b1.dateAdded, b1.lastModified
         FROM moz_bookmarks b1 WHERE b1.guid=:item_guid
         UNION ALL
         SELECT b2.fk, level + 1, b2.type, b2.id, b2.guid, b2.parent,
                descendants.guid, b2.position, b2.title, b2.dateAdded,
                b2.lastModified
         FROM moz_bookmarks b2
         JOIN descendants ON b2.parent = descendants.id AND b2.id <> :tags_folder)
       SELECT d.level, d.id, d.guid, d.parent, d.parentGuid, d.type,
              d.position AS [index], d.title, d.dateAdded, d.lastModified,
              h.url, f.url AS iconuri,
              (SELECT GROUP_CONCAT(t.title, ',')
               FROM moz_bookmarks b2
               JOIN moz_bookmarks t ON t.id = +b2.parent AND t.parent = :tags_folder
               WHERE b2.fk = h.id
              ) AS tags,
              EXISTS (SELECT 1 FROM moz_items_annos
                      WHERE item_id = d.id LIMIT 1) AS has_annos,
              (SELECT a.content FROM moz_annos a
               JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id
               WHERE place_id = h.id AND n.name = :charset_anno
              ) AS charset
       FROM descendants d
       LEFT JOIN moz_bookmarks b3 ON b3.id = d.parent
       LEFT JOIN moz_places h ON h.id = d.fk
       LEFT JOIN moz_favicons f ON f.id = h.favicon_id
       ORDER BY d.level, d.parent, d.position`;


    if (!aItemGuid)
      aItemGuid = this.bookmarks.rootGuid;

    let hasExcludeItemsCallback =
      aOptions.hasOwnProperty("excludeItemsCallback");
    let excludedParents = new Set();
    let shouldExcludeItem = (aItem, aParentGuid) => {
      let exclude = excludedParents.has(aParentGuid) ||
                    aOptions.excludeItemsCallback(aItem);
      if (exclude) {
        if (aItem.type == this.TYPE_X_MOZ_PLACE_CONTAINER)
          excludedParents.add(aItem.guid);
      }
      return exclude;
    };

    let rootItem = null, rootItemCreationEx = null;
    let parentsMap = new Map();
    try {
      let conn = yield this.promiseDBConnection();
      yield conn.executeCached(QUERY_STR,
          { tags_folder: PlacesUtils.tagsFolderId,
            charset_anno: PlacesUtils.CHARSET_ANNO,
            item_guid: aItemGuid }, (aRow) => {
        let item;
        if (!rootItem) {
          
          try {
            rootItem = item = createItemInfoObject(aRow, true);
          }
          catch(ex) {
            
            
            rootItemCreationEx = ex;
            throw StopIteration;
          }

          Object.defineProperty(rootItem, "itemsCount",
                                { value: 1
                                , writable: true
                                , enumerable: false
                                , configurable: false });
        }
        else {
          
          
          item = createItemInfoObject(aRow, false);
          let parentGuid = aRow.getResultByName("parentGuid");
          if (hasExcludeItemsCallback && shouldExcludeItem(item, parentGuid))
            return;

          let parentItem = parentsMap.get(parentGuid);
          if ("children" in parentItem)
            parentItem.children.push(item);
          else
            parentItem.children = [item];

          rootItem.itemsCount++;
        }

        if (item.type == this.TYPE_X_MOZ_PLACE_CONTAINER)
          parentsMap.set(item.guid, item);
      });
    } catch(e) {
      throw new Error("Unable to query the database " + e);
    }
    if (rootItemCreationEx) {
      throw new Error("Failed to fetch the data for the root item" +
                      rootItemCreationEx);
    }

    return rootItem;
  })
};

XPCOMUtils.defineLazyGetter(PlacesUtils, "history", function() {
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"]
             .getService(Ci.nsINavHistoryService)
             .QueryInterface(Ci.nsIBrowserHistory)
             .QueryInterface(Ci.nsPIPlacesDatabase);
  return Object.freeze(new Proxy(hs, {
    get: function(target, name) {
      let property, object;
      if (name in target) {
        property = target[name];
        object = target;
      } else {
        property = History[name];
        object = History;
      }
      if (typeof property == "function") {
        return property.bind(object);
      }
      return property;
    }
  }));
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

XPCOMUtils.defineLazyGetter(PlacesUtils, "bookmarks", () => {
  let bm = Cc["@mozilla.org/browser/nav-bookmarks-service;1"]
             .getService(Ci.nsINavBookmarksService);
  return Object.freeze(new Proxy(bm, {
    get: (target, name) => target.hasOwnProperty(name) ? target[name]
                                                       : Bookmarks[name]
  }));
});

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "annotations",
                                   "@mozilla.org/browser/annotation-service;1",
                                   "nsIAnnotationService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "tagging",
                                   "@mozilla.org/browser/tagging-service;1",
                                   "nsITaggingService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "livemarks",
                                   "@mozilla.org/browser/livemark-service;2",
                                   "mozIAsyncLivemarks");

XPCOMUtils.defineLazyGetter(PlacesUtils, "keywords", () => Keywords);

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

XPCOMUtils.defineLazyGetter(this, "gAsyncDBConnPromised",
  () => new Promise((resolve) => {
    Sqlite.cloneStorageConnection({
      connection: PlacesUtils.history.DBConnection,
      readOnly:   true
    }).then(conn => {
      try {
        Sqlite.shutdown.addBlocker(
          "PlacesUtils read-only connection closing",
          conn.close.bind(conn));
      } catch(ex) {
        
        conn.close();
        throw ex;
      }
      resolve(conn);
    });
  })
);

XPCOMUtils.defineLazyGetter(this, "gAsyncDBWrapperPromised",
  () => new Promise((resolve) => {
    Sqlite.wrapStorageConnection({
      connection: PlacesUtils.history.DBConnection,
    }).then(conn => {
      try {
        Sqlite.shutdown.addBlocker(
          "PlacesUtils wrapped connection closing",
          conn.close.bind(conn));
      } catch(ex) {
        
        conn.close();
        throw ex;
      }
      resolve(conn);
    });
  })
);








let Keywords = {
  












  fetch(keywordOrEntry, onResult=null) {
    if (typeof(keywordOrEntry) == "string")
      keywordOrEntry = { keyword: keywordOrEntry };

    if (keywordOrEntry === null || typeof(keywordOrEntry) != "object" ||
        (("keyword" in keywordOrEntry) && typeof(keywordOrEntry.keyword) != "string"))
      throw new Error("Invalid keyword");

    let hasKeyword = "keyword" in keywordOrEntry;
    let hasUrl = "url" in keywordOrEntry;

    if (!hasKeyword && !hasUrl)
      throw new Error("At least keyword or url must be provided");
    if (onResult && typeof onResult != "function")
      throw new Error("onResult callback must be a valid function");

    if (hasUrl)
      keywordOrEntry.url = new URL(keywordOrEntry.url);
    if (hasKeyword)
      keywordOrEntry.keyword = keywordOrEntry.keyword.trim().toLowerCase();

    let safeOnResult = entry => {
      if (onResult) {
        try {
          onResult(entry);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }
    };

    return gKeywordsCachePromise.then(cache => {
      let entries = [];
      if (hasKeyword) {
        let entry = cache.get(keywordOrEntry.keyword);
        if (entry)
          entries.push(entry);
      }
      if (hasUrl) {
        for (let entry of cache.values()) {
          if (entry.url.href == keywordOrEntry.url.href)
            entries.push(entry);
        }
      }

      entries = entries.filter(e => {
        return (!hasUrl || e.url.href == keywordOrEntry.url.href) &&
               (!hasKeyword || e.keyword == keywordOrEntry.keyword);
      });

      entries.forEach(safeOnResult);
      return entries.length ? entries[0] : null;
    });
  },

  












  insert(keywordEntry) {
    if (!keywordEntry || typeof keywordEntry != "object")
      throw new Error("Input should be a valid object");

    if (!("keyword" in keywordEntry) || !keywordEntry.keyword ||
        typeof(keywordEntry.keyword) != "string")
      throw new Error("Invalid keyword");
    if (("postData" in keywordEntry) && keywordEntry.postData &&
                                        typeof(keywordEntry.postData) != "string")
      throw new Error("Invalid POST data");
    if (!("url" in keywordEntry))
      throw new Error("undefined is not a valid URL");
    let { keyword, url } = keywordEntry;
    keyword = keyword.trim().toLowerCase();
    let postData = keywordEntry.postData || null;
    
    url = new URL(url);

    return Task.spawn(function* () {
      let cache = yield gKeywordsCachePromise;

      
      let oldEntry = cache.get(keyword);
      if (oldEntry && oldEntry.url.href == url.href &&
                      oldEntry.postData == keywordEntry.postData) {
        return;
      }

      
      
      
      
      
      let db = yield PlacesUtils.promiseWrappedConnection();
      if (oldEntry) {
        yield db.executeCached(
          `UPDATE moz_keywords
           SET place_id = (SELECT id FROM moz_places WHERE url = :url),
               post_data = :post_data
           WHERE keyword = :keyword
          `, { url: url.href, keyword: keyword, post_data: postData });
        yield notifyKeywordChange(oldEntry.url.href, "");
      } else {
        
        
        yield db.executeCached(
          `INSERT OR IGNORE INTO moz_places (url, rev_host, hidden, frecency, guid)
           VALUES (:url, :rev_host, 0, :frecency, GENERATE_GUID())
          `, { url: url.href, rev_host: PlacesUtils.getReversedHost(url),
               frecency: url.protocol == "place:" ? 0 : -1 });
        yield db.executeCached(
          `INSERT INTO moz_keywords (keyword, place_id, post_data)
           VALUES (:keyword, (SELECT id FROM moz_places WHERE url = :url), :post_data)
          `, { url: url.href, keyword: keyword, post_data: postData });
      }

      cache.set(keyword, { keyword, url, postData });

      
      yield notifyKeywordChange(url.href, keyword);
    }.bind(this));
  },

  







  remove(keyword) {
    if (!keyword || typeof(keyword) != "string")
      throw new Error("Invalid keyword");
    keyword = keyword.trim().toLowerCase();
    return Task.spawn(function* () {
      let cache = yield gKeywordsCachePromise;
      if (!cache.has(keyword))
        return;
      let { url } = cache.get(keyword);
      cache.delete(keyword);

      let db = yield PlacesUtils.promiseWrappedConnection();
      yield db.execute(`DELETE FROM moz_keywords WHERE keyword = :keyword`,
                       { keyword });

      
      yield notifyKeywordChange(url.href, "");
    }.bind(this));
  }
};



let gIgnoreKeywordNotifications = false;

XPCOMUtils.defineLazyGetter(this, "gKeywordsCachePromise", Task.async(function* () {
  let cache = new Map();
  let db = yield PlacesUtils.promiseWrappedConnection();
  let rows = yield db.execute(
    `SELECT keyword, url, post_data
     FROM moz_keywords k
     JOIN moz_places h ON h.id = k.place_id
    `);
  for (let row of rows) {
    let keyword = row.getResultByName("keyword");
    let entry = { keyword,
                  url: new URL(row.getResultByName("url")),
                  postData: row.getResultByName("post_data") };
    cache.set(keyword, entry);
  }

  
  function keywordsForHref(href) {
    let keywords = [];
    for (let [ key, val ] of cache) {
      if (val.url.href == href)
        keywords.push(key);
    }
    return keywords;
  }

  
  
  
  let observer = {
    QueryInterface: XPCOMUtils.generateQI(Ci.nsINavBookmarkObserver),
    onBeginUpdateBatch() {},
    onEndUpdateBatch() {},
    onItemAdded() {},
    onItemVisited() {},
    onItemMoved() {},

    onItemRemoved(id, parentId, index, itemType, uri, guid, parentGuid) {
      if (itemType != PlacesUtils.bookmarks.TYPE_BOOKMARK)
        return;

      let keywords = keywordsForHref(uri.spec);
      
      if (keywords.length == 0)
        return;

      Task.spawn(function* () {
        
        let bookmark = yield PlacesUtils.bookmarks.fetch({ url: uri });
        if (!bookmark) {
          for (let keyword of keywords) {
            yield PlacesUtils.keywords.remove(keyword);
          }
        }
      }).catch(Cu.reportError);
    },

    onItemChanged(id, prop, isAnno, val, lastMod, itemType, parentId, guid) {
      if (gIgnoreKeywordNotifications ||
          prop != "keyword")
        return;

      Task.spawn(function* () {
        let bookmark = yield PlacesUtils.bookmarks.fetch(guid);
        
        if (!bookmark)
          return;

        if (val.length == 0) {
          
          let keywords = keywordsForHref(bookmark.url.href)
          for (let keyword of keywords) {
            cache.delete(keyword);
          }
        } else {
          
          cache.set(val, { keyword: val, url: bookmark.url });
        }
      }).catch(Cu.reportError);
    }
  };

  PlacesUtils.bookmarks.addObserver(observer, false);
  PlacesUtils.registerShutdownFunction(() => {
    PlacesUtils.bookmarks.removeObserver(observer);
  });
  return cache;
}));















let GuidHelper = {
  
  guidsForIds: new Map(),
  idsForGuids: new Map(),

  getItemId: Task.async(function* (aGuid) {
    let cached = this.idsForGuids.get(aGuid);
    if (cached !== undefined)
      return cached;

    let conn = yield PlacesUtils.promiseDBConnection();
    let rows = yield conn.executeCached(
      "SELECT b.id, b.guid from moz_bookmarks b WHERE b.guid = :guid LIMIT 1",
      { guid: aGuid });
    if (rows.length == 0)
      throw new Error("no item found for the given GUID");

    this.ensureObservingRemovedItems();
    let itemId = rows[0].getResultByName("id");
    this.idsForGuids.set(aGuid, itemId);
    return itemId;
  }),

  getItemGuid: Task.async(function* (aItemId) {
    let cached = this.guidsForIds.get(aItemId);
    if (cached !== undefined)
      return cached;

    let conn = yield PlacesUtils.promiseDBConnection();
    let rows = yield conn.executeCached(
      "SELECT b.id, b.guid from moz_bookmarks b WHERE b.id = :id LIMIT 1",
      { id: aItemId });
    if (rows.length == 0)
      throw new Error("no item found for the given itemId");

    this.ensureObservingRemovedItems();
    let guid = rows[0].getResultByName("guid");
    this.guidsForIds.set(aItemId, guid);
    return guid;
  }),

  ensureObservingRemovedItems: function () {
    if (!("observer" in this)) {
      






      this.observer = {
        onItemAdded: (aItemId, aParentId, aIndex, aItemType, aURI, aTitle,
                      aDateAdded, aGuid, aParentGuid) => {
          this.guidsForIds.set(aItemId, aGuid);
          this.guidsForIds.set(aParentId, aParentGuid);
        },
        onItemRemoved:
        (aItemId, aParentId, aIndex, aItemTyep, aURI, aGuid, aParentGuid) => {
          this.guidsForIds.delete(aItemId);
          this.idsForGuids.delete(aGuid);
          this.guidsForIds.set(aParentId, aParentGuid);
        },

        QueryInterface: XPCOMUtils.generateQI(Ci.nsINavBookmarkObserver),

        onBeginUpdateBatch: function() {},
        onEndUpdateBatch: function() {},
        onItemChanged: function() {},
        onItemVisited: function() {},
        onItemMoved: function() {},
      };
      PlacesUtils.bookmarks.addObserver(this.observer, false);
      PlacesUtils.registerShutdownFunction(() => {
        PlacesUtils.bookmarks.removeObserver(this.observer);
      });
    }
  }
};








function updateCommandsOnActiveWindow()
{
  let win = Services.focus.activeWindow;
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
    this._uri = (v instanceof Ci.nsIURI ? v.clone() : null),
  get uri()
    this._uri || null,
  set feedURI(v)
    this._feedURI = (v instanceof Ci.nsIURI ? v.clone() : null),
  get feedURI()
    this._feedURI || null,
  set siteURI(v)
    this._siteURI = (v instanceof Ci.nsIURI ? v.clone() : null),
  get siteURI()
    this._siteURI || null,
  set index(v)
    this._index = (parseInt(v) >= 0 ? v : null),
  
  get index()
    this._index != null ? this._index : PlacesUtils.bookmarks.DEFAULT_INDEX,
  set annotations(v)
    this._annotations = Array.isArray(v) ? Cu.cloneInto(v, {}) : null,
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
      }).then(aLivemark => {
        this.item.id = aLivemark.id;
        if (this.item.annotations && this.item.annotations.length > 0) {
          PlacesUtils.setAnnotationsForItem(this.item.id,
                                            this.item.annotations);
        }
      }, Cu.reportError);
  },

  undoTransaction: function CLTXN_undoTransaction()
  {
    
    
    PlacesUtils.livemarks.getLivemark({ id: this.item.id })
      .then(null, null).then( () => {
        PlacesUtils.bookmarks.removeItem(this.item.id);
      });
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
    PlacesUtils.livemarks.getLivemark({ id: this.item.id })
      .then(aLivemark => {
        this.item.feedURI = aLivemark.feedURI;
        this.item.siteURI = aLivemark.siteURI;
        PlacesUtils.bookmarks.removeItem(this.item.id);
      }, Cu.reportError);
  },

  undoTransaction: function RLTXN_undoTransaction()
  {
    
    
    
    
    PlacesUtils.livemarks.getLivemark({ id: this.item.id })
      .then(null, () => {
        PlacesUtils.livemarks.addLivemark({ parentId: this.item.parentId
                                          , title: this.item.title
                                          , siteURI: this.item.siteURI
                                          , feedURI: this.item.feedURI
                                          , index: this.item.index
                                          , lastModified: this.item.lastModified
                                          }).then(
          aLivemark => {
            let itemId = aLivemark.id;
            PlacesUtils.bookmarks.setItemDateAdded(itemId, this.item.dateAdded);
            PlacesUtils.setAnnotationsForItem(itemId, this.item.annotations);
          }, Cu.reportError);
      });
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
      PlacesUtils.tagging.tagURI(this.new.uri, this.item.tags);
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
      
      let flags = {}, expires = {}, type = {};
      PlacesUtils.annotations.getItemAnnotationInfo(this.item.id, annoName, flags,
                                                    expires, type);
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
      
      let flags = {}, expires = {}, type = {};
      PlacesUtils.annotations.getPageAnnotationInfo(this.item.uri, annoName, flags,
                                                    expires, type);
      let value = PlacesUtils.annotations.getPageAnnotation(this.item.uri,
                                                            annoName);
      this.item.annotations = [{ name: annoName,
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

  doTransaction() {
    
    if (this.new.postData) {
      this.item.postData = PlacesUtils.getPostDataForBookmark(this.item.id);
      PlacesUtils.setPostDataForBookmark(this.item.id, this.new.postData);
    }
  },

  undoTransaction() {
    
    if (this.item.postData) {
      PlacesUtils.setPostDataForBookmark(this.item.id, this.item.postData);
    }
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
        for (let item in this._self._oldOrder)
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
