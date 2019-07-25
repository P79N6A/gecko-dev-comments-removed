









































const EXPORTED_SYMBOLS = [
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
, "PlacesEditLivemarkSiteURITransaction"
, "PlacesEditLivemarkFeedURITransaction"
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

XPCOMUtils.defineLazyGetter(this, "Services", function() {
  Cu.import("resource://gre/modules/Services.jsm");
  return Services;
});

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});




const MIN_TRANSACTIONS_FOR_BATCH = 5;



const RESTORE_BEGIN_NSIOBSERVER_TOPIC = "bookmarks-restore-begin";
const RESTORE_SUCCESS_NSIOBSERVER_TOPIC = "bookmarks-restore-success";
const RESTORE_FAILED_NSIOBSERVER_TOPIC = "bookmarks-restore-failed";
const RESTORE_NSIOBSERVER_DATA = "json";

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
function asVisit(aNode) QI_node(aNode, Ci.nsINavHistoryVisitResultNode);
function asFullVisit(aNode) QI_node(aNode, Ci.nsINavHistoryFullVisitResultNode);
function asContainer(aNode) QI_node(aNode, Ci.nsINavHistoryContainerResultNode);
function asQuery(aNode) QI_node(aNode, Ci.nsINavHistoryQueryResultNode);

var PlacesUtils = {
  
  TYPE_X_MOZ_PLACE_CONTAINER: "text/x-moz-place-container",
  
  TYPE_X_MOZ_PLACE_SEPARATOR: "text/x-moz-place-separator",
  
  TYPE_X_MOZ_PLACE: "text/x-moz-place",
  
  TYPE_X_MOZ_URL: "text/x-moz-url",
  
  TYPE_HTML: "text/html",
  
  TYPE_UNICODE: "text/unicode",

  EXCLUDE_FROM_BACKUP_ANNO: "places/excludeFromBackup",
  GUID_ANNO: "placesInternal/GUID",
  LMANNO_FEEDURI: "livemark/feedURI",
  LMANNO_SITEURI: "livemark/siteURI",
  LMANNO_EXPIRATION: "livemark/expiration",
  LMANNO_LOADFAILED: "livemark/loadfailed",
  LMANNO_LOADING: "livemark/loading",
  POST_DATA_ANNO: "bookmarkProperties/POSTData",
  READ_ONLY_ANNO: "placesInternal/READ_ONLY",

  TOPIC_SHUTDOWN: "places-shutdown",
  TOPIC_INIT_COMPLETE: "places-init-complete",
  TOPIC_DATABASE_LOCKED: "places-database-locked",
  TOPIC_EXPIRATION_FINISHED: "places-expiration-finished",
  TOPIC_FEEDBACK_UPDATED: "places-autocomplete-feedback-updated",
  TOPIC_FAVICONS_EXPIRED: "places-favicons-expired",
  TOPIC_VACUUM_STARTING: "places-vacuum-starting",

  asVisit: function(aNode) asVisit(aNode),
  asFullVisit: function(aNode) asFullVisit(aNode),
  asContainer: function(aNode) asContainer(aNode),
  asQuery: function(aNode) asQuery(aNode),

  endl: NEWLINE,

  





  _uri: function PU__uri(aSpec) {
    return NetUtil.newURI(aSpec);
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

    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR);
  },

  





  nodeIsVisit: function PU_nodeIsVisit(aNode) {
    var type = aNode.type;
    return type == Ci.nsINavHistoryResultNode.RESULT_TYPE_VISIT ||
           type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FULL_VISIT;
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
    if (aTopic == this.TOPIC_SHUTDOWN) {
      Services.obs.removeObserver(this, this.TOPIC_SHUTDOWN);
      this._shutdownFunctions.forEach(function (aFunc) aFunc.apply(this), this);
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
    if (this.nodeIsFolder(aNode) || this.nodeIsDynamicContainer(aNode)) {
      if (this._readOnly.indexOf(aNode.itemId) != -1)
        return true;
    }
    else if (this.nodeIsQuery(aNode) &&
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
    if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_DYNAMIC_CONTAINER)
      return true;
    return false;
  },

  





  itemIsLivemark: function PU_itemIsLivemark(aItemId) {
    
    
    
    if (Object.getOwnPropertyDescriptor(this, "livemarks").value === undefined)
      return this.annotations.itemHasAnnotation(aItemId, this.LMANNO_FEEDURI);
    
    return this.livemarks.isLivemark(aItemId);
  },

  





  nodeIsLivemarkContainer: function PU_nodeIsLivemarkContainer(aNode) {
    return this.nodeIsFolder(aNode) && this.itemIsLivemark(aNode.itemId);
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
        this.serializeNodeAsJSONToOutputStream(node, writer, true, aForceCopy);
        if (shouldClose)
          node.containerOpen = false;

        return writer.value;
      }
      case this.TYPE_X_MOZ_URL: {
        function gatherDataUrl(bNode) {
          if (PlacesUtils.nodeIsLivemarkContainer(bNode)) {
            let siteURI = PlacesUtils.livemarks.getSiteURI(bNode.itemId).spec;
            return siteURI + NEWLINE + bNode.title;
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
          if (PlacesUtils.nodeIsLivemarkContainer(bNode)) {
            let siteURI = PlacesUtils.livemarks.getSiteURI(bNode.itemId).spec;
            return "<A HREF=\"" + siteURI + "\">" + escapedTitle + "</A>" + NEWLINE;
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
      if (PlacesUtils.nodeIsLivemarkContainer(bNode))
        return PlacesUtils.livemarks.getSiteURI(bNode.itemId).spec;
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
        var json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
        
        
        
        
        
        nodes = json.legacyDecode("[" + blob + "]");
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
      
      if (this.itemIsLivemark(parentId))
        return false;
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
      if (grandparentId != this.tagsFolderId &&
          !this.itemIsLivemark(parentId))
        return itemId;
    }
    return -1;
  },

  






  getMostRecentFolderForFeedURI:
  function PU_getMostRecentFolderForFeedURI(aFeedURI) {
    
    
    
    if (Object.getOwnPropertyDescriptor(this, "livemarks").value === undefined) {
      var feedSpec = aFeedURI.spec
      var annosvc = this.annotations;
      var livemarks = annosvc.getItemsWithAnnotation(this.LMANNO_FEEDURI);
      for (var i = 0; i < livemarks.length; i++) {
        if (annosvc.getItemAnnotation(livemarks[i], this.LMANNO_FEEDURI) == feedSpec)
          return livemarks[i];
      }
    }
    else {
      
      return this.livemarks.getLivemarkIdForFeedURI(aFeedURI);
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

  









  restoreBookmarksFromJSONString:
  function PU_restoreBookmarksFromJSONString(aString, aReplace) {
    
    var nodes = this.unwrapNodes(aString, this.TYPE_X_MOZ_PLACE_CONTAINER);

    if (nodes.length == 0 || !nodes[0].children ||
        nodes[0].children.length == 0)
      return; 

    
    nodes[0].children.sort(function sortRoots(aNode, bNode) {
      return (aNode.root && aNode.root == "tagsFolder") ? 1 :
              (bNode.root && bNode.root == "tagsFolder") ? -1 : 0;
    });

    var batch = {
      nodes: nodes[0].children,
      runBatched: function restore_runBatched() {
        if (aReplace) {
          
          
          var excludeItems = PlacesUtils.annotations.getItemsWithAnnotation(
            PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO
          );
          
          
          
          var query = PlacesUtils.history.getNewQuery();
          query.setFolders([PlacesUtils.placesRootId], 1);
          var options = PlacesUtils.history.getNewQueryOptions();
          options.expandQueries = false;
          var root = PlacesUtils.history.executeQuery(query, options).root;
          root.containerOpen = true;
          var childIds = [];
          for (var i = 0; i < root.childCount; i++) {
            var childId = root.getChild(i).itemId;
            if (excludeItems.indexOf(childId) == -1 &&
                childId != PlacesUtils.tagsFolderId)
              childIds.push(childId);
          }
          root.containerOpen = false;

          for (var i = 0; i < childIds.length; i++) {
            var rootItemId = childIds[i];
            if (PlacesUtils.isRootItem(rootItemId))
              PlacesUtils.bookmarks.removeFolderChildren(rootItemId);
            else
              PlacesUtils.bookmarks.removeItem(rootItemId);
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
              for (var i = 0; i < folders.length; i++) {
                if (folders[i])
                  folderIdMap[i] = folders[i];
              }
              searchIds = searchIds.concat(searches);
            }, this);
          }
          else
            this.importJSONNode(node, this.placesRootId, node.index);

        }, PlacesUtils);

        
        searchIds.forEach(function(aId) {
          var oldURI = this.bookmarks.getBookmarkURI(aId);
          var uri = this._fixupQuery(this.bookmarks.getBookmarkURI(aId),
                                     folderIdMap);
          if (!uri.equals(oldURI))
            this.bookmarks.changeBookmarkURI(aId, uri);
        }, PlacesUtils);
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
              case this.LMANNO_EXPIRATION:
              case this.LMANNO_LOADING:
              case this.LMANNO_LOADFAILED:
                return false;
              default:
                return true;
            }
          }, this);

          if (feedURI) {
            id = this.livemarks.createLivemarkFolderOnly(aContainer,
                                                         aData.title,
                                                         siteURI, feedURI,
                                                         aIndex);
          }
        }
        else {
          id = this.bookmarks.createFolder(aContainer, aData.title, aIndex);
          folderIdMap[aData.id] = id;
          
          if (aData.children) {
            aData.children.forEach(function(aChild, aIndex) {
              var [folders, searches] = this.importJSONNode(aChild, id, aIndex);
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
        if (aData.charset)
          this.history.setCharsetForURI(this._uri(aData.uri), aData.charset);
        if (aData.uri.substr(0, 6) == "place:")
          searchIds.push(id);
        if (aData.icon) {
          try {
            
            let faviconURI = this._uri("fake-favicon-uri:" + aData.uri);
            this.favicons.setFaviconUrlForPage(this._uri(aData.uri), faviconURI);
            this.favicons.setFaviconDataFromDataURL(faviconURI, aData.icon, 0);
          } catch (ex) {
            Components.utils.reportError("Failed to import favicon data:"  + ex);
          }
        }
        if (aData.iconUri) {
          try {
            this.favicons.setAndLoadFaviconForPage(this._uri(aData.uri),
                                                   this._uri(aData.iconUri),
                                                   false);
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

    
    if (id != -1) {
      if (aData.dateAdded)
        this.bookmarks.setItemDateAdded(id, aData.dateAdded);
      if (aData.lastModified)
        this.bookmarks.setItemLastModified(id, aData.lastModified);
      if (aData.annos && aData.annos.length)
        this.setAnnotationsForItem(id, aData.annos);
    }

    return [folderIdMap, searchIds];
  },

  










  _fixupQuery: function PU__fixupQuery(aQueryURI, aFolderIdMap) {
    function convert(str, p1, offset, s) {
      return "folder=" + aFolderIdMap[p1];
    }
    var stringURI = aQueryURI.spec.replace(/folder=([0-9]+)/g, convert);
    return this._uri(stringURI);
  },

  

















  serializeNodeAsJSONToOutputStream:
  function PU_serializeNodeAsJSONToOutputStream(aNode, aStream, aIsUICommand,
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
      var lastCharset = PlacesUtils.history.getCharsetForURI(uri);
      if (lastCharset)
        aJSNode.charset = lastCharset;
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

  


  toJSONString: function PU_toJSONString(aObj) {
    var JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    return JSON.encode(aObj);
  },

  







  restoreBookmarksFromJSONFile:
  function PU_restoreBookmarksFromJSONFile(aFile) {
    let failed = false;
    Services.obs.notifyObservers(null,
                                 RESTORE_BEGIN_NSIOBSERVER_TOPIC,
                                 RESTORE_NSIOBSERVER_DATA);

    try {
      
      var stream = Cc["@mozilla.org/network/file-input-stream;1"].
                   createInstance(Ci.nsIFileInputStream);
      stream.init(aFile, 0x01, 0, 0);
      var converted = Cc["@mozilla.org/intl/converter-input-stream;1"].
                      createInstance(Ci.nsIConverterInputStream);
      converted.init(stream, "UTF-8", 8192,
                     Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);

      
      var str = {};
      var jsonStr = "";
      while (converted.readString(8192, str) != 0)
        jsonStr += str.value;
      converted.close();

      if (jsonStr.length == 0)
        return; 

      this.restoreBookmarksFromJSONString(jsonStr, true);
    }
    catch (exc) {
      failed = true;
      Services.obs.notifyObservers(null,
                                   RESTORE_FAILED_NSIOBSERVER_TOPIC,
                                   RESTORE_NSIOBSERVER_DATA);
      Cu.reportError("Bookmarks JSON restore failed: " + exc);
      throw exc;
    }
    finally {
      if (!failed) {
        Services.obs.notifyObservers(null,
                                     RESTORE_SUCCESS_NSIOBSERVER_TOPIC,
                                     RESTORE_NSIOBSERVER_DATA);
      }
    }
  },

  




  backupBookmarksToFile: function PU_backupBookmarksToFile(aFile) {
    this.backups.saveBookmarksToJSONFile(aFile);
  },

  





  archiveBookmarksFile:
  function PU_archiveBookmarksFile(aMaxBackups, aForceBackup) {
    this.backups.create(aMaxBackups, aForceBackup);
  },

  


  backups: {

    get _filenamesRegex() {
      
      
      let localizedFilename =
        PlacesUtils.getFormattedString("bookmarksArchiveFilename", [new Date()]);
      let localizedFilenamePrefix =
        localizedFilename.substr(0, localizedFilename.indexOf("-"));
      delete this._filenamesRegex;
      return this._filenamesRegex =
        new RegExp("^(bookmarks|" + localizedFilenamePrefix + ")-([0-9-]+)\.(json|html)");
    },

    get folder() {
      let bookmarksBackupDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
      bookmarksBackupDir.append("bookmarkbackups");
      if (!bookmarksBackupDir.exists()) {
        bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
        if (!bookmarksBackupDir.exists())
          throw("Unable to create bookmarks backup folder");
      }
      delete this.folder;
      return this.folder = bookmarksBackupDir;
    },

    


    get entries() {
      delete this.entries;
      this.entries = [];
      let files = this.folder.directoryEntries;
      while (files.hasMoreElements()) {
        let entry = files.getNext().QueryInterface(Ci.nsIFile);
        
        
        let matches = entry.leafName.match(this._filenamesRegex);
        if (!entry.isHidden() && matches) {
          
          if (this.getDateForFile(entry) > new Date()) {
            entry.remove(false);
            continue;
          }
          this.entries.push(entry);
        }
      }
      this.entries.sort(function compare(a, b) {
        let aDate = PlacesUtils.backups.getDateForFile(a);
        let bDate = PlacesUtils.backups.getDateForFile(b);
        return aDate < bDate ? 1 : aDate > bDate ? -1 : 0;
      });
      return this.entries;
    },

    







    getFilenameForDate:
    function PU_B_getFilenameForDate(aDateObj) {
      let dateObj = aDateObj || new Date();
      
      
      return "bookmarks-" + dateObj.toLocaleFormat("%Y-%m-%d") + ".json";
    },

    







    getDateForFile:
    function PU_B_getDateForFile(aBackupFile) {
      let filename = aBackupFile.leafName;
      let matches = filename.match(this._filenamesRegex);
      if (!matches)
        do_throw("Invalid backup file name: " + filename);
      return new Date(matches[2].replace(/-/g, "/"));
    },

    







    getMostRecent:
    function PU__B_getMostRecent(aFileExt) {
      let fileExt = aFileExt || "(json|html)";
      for (let i = 0; i < this.entries.length; i++) {
        let rx = new RegExp("\." + fileExt + "$");
        if (this.entries[i].leafName.match(rx))
          return this.entries[i];
      }
      return null;
    },

    









    saveBookmarksToJSONFile:
    function PU_B_saveBookmarksToFile(aFile) {
      if (!aFile.exists())
        aFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
      if (!aFile.exists() || !aFile.isWritable()) {
        Cu.reportError("Unable to create bookmarks backup file: " + aFile.leafName);
        return;
      }

      this._writeBackupFile(aFile);

      if (aFile.parent.equals(this.folder)) {
        
        this.entries.push(aFile);
      }
      else {
        
        
        
        
        var latestBackup = this.getMostRecent("json");
        if (!latestBackup || latestBackup != aFile) {
          let name = this.getFilenameForDate();
          let file = this.folder.clone();
          file.append(name);
          if (file.exists())
            file.remove(false);
          else {
            
            
            this.entries.push(file);
          }
          aFile.copyTo(this.folder, name);
        }
      }
    },

    _writeBackupFile:
    function PU_B__writeBackupFile(aFile) {
      
      let stream = Cc["@mozilla.org/network/file-output-stream;1"].
                   createInstance(Ci.nsIFileOutputStream);
      stream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

      
      let converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
                   createInstance(Ci.nsIConverterOutputStream);
      converter.init(stream, "UTF-8", 0, 0x0000);

      
      let streamProxy = {
        converter: converter,
        write: function(aData, aLen) {
          this.converter.writeString(aData);
        }
      };

      
      let excludeItems =
        PlacesUtils.annotations.getItemsWithAnnotation(PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO);

      
      let options = PlacesUtils.history.getNewQueryOptions();
      options.expandQueries = false;
      let query = PlacesUtils.history.getNewQuery();
      query.setFolders([PlacesUtils.placesRootId], 1);
      let root = PlacesUtils.history.executeQuery(query, options).root;
      root.containerOpen = true;
      
      PlacesUtils.serializeNodeAsJSONToOutputStream(root, streamProxy,
                                                    false, false, excludeItems);
      root.containerOpen = false;

      
      converter.close();
      stream.close();
    },

    














    create:
    function PU_B_create(aMaxBackups, aForceBackup) {
      
      let newBackupFilename = this.getFilenameForDate();
      let mostRecentBackupFile = this.getMostRecent();

      if (!aForceBackup) {
        let numberOfBackupsToDelete = 0;
        if (aMaxBackups !== undefined && aMaxBackups > -1)
          numberOfBackupsToDelete = this.entries.length - aMaxBackups;

        if (numberOfBackupsToDelete > 0) {
          
          
          
          if (!mostRecentBackupFile ||
              mostRecentBackupFile.leafName != newBackupFilename)
            numberOfBackupsToDelete++;

          while (numberOfBackupsToDelete--) {
            let oldestBackup = this.entries.pop();
            oldestBackup.remove(false);
          }
        }

        
        if (aMaxBackups === 0 ||
            (mostRecentBackupFile &&
             mostRecentBackupFile.leafName == newBackupFilename))
          return;
      }

      let newBackupFile = this.folder.clone();
      newBackupFile.append(newBackupFilename);

      if (aForceBackup && newBackupFile.exists())
        newBackupFile.remove(false);

      if (newBackupFile.exists())
        return;

      this.saveBookmarksToJSONFile(newBackupFile);
    }

  },

  
















  asyncGetBookmarkIds: function PU_asyncGetBookmarkIds(aURI, aCallback, aScope)
  {
    if (!this._asyncGetBookmarksStmt) {
      let db = this.history.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
      this._asyncGetBookmarksStmt = db.createAsyncStatement(
        "SELECT b.id "
      + "FROM moz_bookmarks b "
      + "JOIN moz_places h on h.id = b.fk "
      + "WHERE h.url = :url "
      +   "AND NOT EXISTS( "
      +     "SELECT 1 FROM moz_items_annos a "
      +     "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
      +     "WHERE a.item_id = b.parent AND n.name = :name "
      +   ") "
      );
      this.registerShutdownFunction(function () {
        this._asyncGetBookmarksStmt.finalize();
      });
    }

    let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
    this._asyncGetBookmarksStmt.params.url = url;
    this._asyncGetBookmarksStmt.params.name = this.LMANNO_FEEDURI;

    
    
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

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "history",
                                   "@mozilla.org/browser/nav-history-service;1",
                                   "nsINavHistoryService");

XPCOMUtils.defineLazyGetter(PlacesUtils, "bhistory", function() {
  return PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
});

XPCOMUtils.defineLazyGetter(PlacesUtils, "ghistory2", function() {
  return PlacesUtils.history.QueryInterface(Ci.nsIGlobalHistory2);
});

XPCOMUtils.defineLazyGetter(PlacesUtils, "ghistory3", function() {
  return PlacesUtils.history.QueryInterface(Ci.nsIGlobalHistory3);
});

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "favicons",
                                   "@mozilla.org/browser/favicon-service;1",
                                   "nsIFaviconService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "bookmarks",
                                   "@mozilla.org/browser/nav-bookmarks-service;1",
                                   "nsINavBookmarksService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "annotations",
                                   "@mozilla.org/browser/annotation-service;1",
                                   "nsIAnnotationService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "tagging",
                                   "@mozilla.org/browser/tagging-service;1",
                                   "nsITaggingService");

XPCOMUtils.defineLazyServiceGetter(PlacesUtils, "livemarks",
                                   "@mozilla.org/browser/livemark-service;2",
                                   "nsILivemarkService");

XPCOMUtils.defineLazyGetter(PlacesUtils, "transactionManager", function() {
  let tm = Cc["@mozilla.org/transactionmanager;1"].
           getService(Ci.nsITransactionManager);
  tm.AddListener(PlacesUtils);
  this.registerShutdownFunction(function () {
    
    
    this.transactionManager.RemoveListener(this);
    this.transactionManager.clear();
  });
  return tm;
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
  if (win && win instanceof Ci.nsIDOMWindowInternal) {
    
    win.updateCommands("undo");
  }
}








function BaseTransaction() {}

BaseTransaction.prototype = {
  doTransaction: function BTXN_doTransaction() {},
  redoTransaction: function BTXN_redoTransaction() this.doTransaction(),
  undoTransaction: function BTXN_undoTransaction() {},
  merge: function BTXN_merge() false,
  get isTransient() false,
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITransaction
  ]),
};












function PlacesAggregatedTransaction(aName, aTransactions)
{
  this._transactions = aTransactions;
  this._name = aName;
  this.container = -1;

  
  
  let countTransactions = function(aTransactions, aTxnCount)
  {
    for (let i = 0;
         i < aTransactions.length && aTxnCount < MIN_TRANSACTIONS_FOR_BATCH;
         ++i, ++aTxnCount) {
      let txn = aTransactions[i];
      if (txn && txn.childTransactions && txn.childTransactions.length)
        aTxnCount = countTransactions(txn.childTransactions, aTxnCount);
    }
    return aTxnCount;
  }

  let txnCount = countTransactions(this._transactions, 0);
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
    
    
    let transactions = this._transactions.slice(0);
    if (this._isUndo)
      transactions.reverse();
    for (let i = 0; i < transactions.length; ++i) {
      let txn = transactions[i];
      if (this.container > -1)
        txn.container = this.container;
      if (this._isUndo)
        txn.undoTransaction();
      else
        txn.doTransaction();
    }
  }
};




















function PlacesCreateFolderTransaction(aName, aContainer, aIndex, aAnnotations,
                                       aChildItemsTransactions)
{
  this._name = aName;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._annotations = aAnnotations;
  this._id = null;
  this.childTransactions = aChildItemsTransactions || [];
}

PlacesCreateFolderTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this._container,
  set container(val) this._container = val,

  doTransaction: function CFTXN_doTransaction()
  {
    this._id = PlacesUtils.bookmarks.createFolder(this._container, 
                                                  this._name, this._index);
    if (this._annotations && this._annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this._id, this._annotations);

    if (this.childTransactions.length) {
      
      for (let i = 0; i < this.childTransactions.length; ++i) {
        this.childTransactions[i].container = this._id;
      }

      let txn = new PlacesAggregatedTransaction("Create folder childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }

    if (this._GUID)
      PlacesUtils.bookmarks.setItemGUID(this._id, this._GUID);
  },

  undoTransaction: function CFTXN_undoTransaction()
  {
    if (this.childTransactions.length) {
      let txn = new PlacesAggregatedTransaction("Create folder childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }

    
    if (PlacesUtils.annotations.itemHasAnnotation(this._id, PlacesUtils.GUID_ANNO))
      this._GUID = PlacesUtils.bookmarks.getItemGUID(this._id);

    
    PlacesUtils.bookmarks.removeItem(this._id);
  }
};




























function PlacesCreateBookmarkTransaction(aURI, aContainer, aIndex, aTitle,
                                         aKeyword, aAnnotations,
                                         aChildTransactions)
{
  this._uri = aURI;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._title = aTitle;
  this._keyword = aKeyword;
  this._annotations = aAnnotations;
  this.childTransactions = aChildTransactions || [];
}

PlacesCreateBookmarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this._container,
  set container(val) this._container = val,

  doTransaction: function CITXN_doTransaction()
  {
    this._id = PlacesUtils.bookmarks.insertBookmark(this.container, this._uri,
                                                    this._index, this._title);
    if (this._keyword)
      PlacesUtils.bookmarks.setKeywordForBookmark(this._id, this._keyword);
    if (this._annotations && this._annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this._id, this._annotations);
 
    if (this.childTransactions.length) {
      
      for (let i = 0; i < this.childTransactions.length; ++i) {
        this.childTransactions[i].id = this._id;
      }
      let txn = new PlacesAggregatedTransaction("Create item childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }
    if (this._GUID)
      PlacesUtils.bookmarks.setItemGUID(this._id, this._GUID);
  },

  undoTransaction: function CITXN_undoTransaction()
  {
    if (this.childTransactions.length) {
      
      let txn = new PlacesAggregatedTransaction("Create item childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }

    
    if (PlacesUtils.annotations.itemHasAnnotation(this._id, PlacesUtils.GUID_ANNO))
      this._GUID = PlacesUtils.bookmarks.getItemGUID(this._id);

    
    PlacesUtils.bookmarks.removeItem(this._id);
  }
};














function PlacesCreateSeparatorTransaction(aContainer, aIndex)
{
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._id = null;
}

PlacesCreateSeparatorTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this._container,
  set container(val) this._container = val,

  doTransaction: function CSTXN_doTransaction()
  {
    this._id = PlacesUtils.bookmarks
                          .insertSeparator(this.container, this._index);
    if (this._GUID)
      PlacesUtils.bookmarks.setItemGUID(this._id, this._GUID);
  },

  undoTransaction: function CSTXN_undoTransaction()
  {
    
    if (PlacesUtils.annotations.itemHasAnnotation(this._id, PlacesUtils.GUID_ANNO))
      this._GUID = PlacesUtils.bookmarks.getItemGUID(this._id);

    PlacesUtils.bookmarks.removeItem(this._id);
  }
};



















function PlacesCreateLivemarkTransaction(aFeedURI, aSiteURI, aName, aContainer,
                                         aIndex, aAnnotations)
{
  this._feedURI = aFeedURI;
  this._siteURI = aSiteURI;
  this._name = aName;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._annotations = aAnnotations;
}

PlacesCreateLivemarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this._container,
  set container(val) this._container = val,

  doTransaction: function CLTXN_doTransaction()
  {
    this._id = PlacesUtils.livemarks.createLivemark(this._container, this._name,
                                                    this._siteURI, this._feedURI,
                                                    this._index);
    if (this._annotations && this._annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this._id, this._annotations);
    if (this._GUID)
      PlacesUtils.bookmarks.setItemGUID(this._id, this._GUID);
  },

  undoTransaction: function CLTXN_undoTransaction()
  {
    
    if (PlacesUtils.annotations.itemHasAnnotation(this._id, PlacesUtils.GUID_ANNO))
      this._GUID = PlacesUtils.bookmarks.getItemGUID(this._id);

    PlacesUtils.bookmarks.removeItem(this._id);
  }
};











function PlacesRemoveLivemarkTransaction(aFolderId)
{
  this._id = aFolderId;
  this._title = PlacesUtils.bookmarks.getItemTitle(this._id);
  this._container = PlacesUtils.bookmarks.getFolderIdForItem(this._id);
  let annos = PlacesUtils.getAnnotationsForItem(this._id);
  
  let annosToExclude = ["livemark/feedURI",
                        "livemark/siteURI",
                        "livemark/expiration",
                        "livemark/loadfailed",
                        "livemark/loading"];
  this._annotations = annos.filter(function(aValue, aIndex, aArray) {
      return annosToExclude.indexOf(aValue.name) == -1;
    });
  this._feedURI = PlacesUtils.livemarks.getFeedURI(this._id);
  this._siteURI = PlacesUtils.livemarks.getSiteURI(this._id);
  this._dateAdded = PlacesUtils.bookmarks.getItemDateAdded(this._id);
  this._lastModified = PlacesUtils.bookmarks.getItemLastModified(this._id);
}

PlacesRemoveLivemarkTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function RLTXN_doTransaction()
  {
    this._index = PlacesUtils.bookmarks.getItemIndex(this._id);
    PlacesUtils.bookmarks.removeItem(this._id);
  },

  undoTransaction: function RLTXN_undoTransaction()
  {
    this._id = PlacesUtils.livemarks.createLivemark(this._container,
                                                    this._title,
                                                    this._siteURI,
                                                    this._feedURI,
                                                    this._index);
    PlacesUtils.bookmarks.setItemDateAdded(this._id, this._dateAdded);
    PlacesUtils.bookmarks.setItemLastModified(this._id, this._lastModified);
    
    PlacesUtils.setAnnotationsForItem(this._id, this._annotations);
  }
};














function PlacesMoveItemTransaction(aItemId, aNewContainer, aNewIndex)
{
  this._id = aItemId;
  this._oldContainer = PlacesUtils.bookmarks.getFolderIdForItem(this._id);
  this._newContainer = aNewContainer;
  this._newIndex = aNewIndex;
}

PlacesMoveItemTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function MITXN_doTransaction()
  {
    this._oldIndex = PlacesUtils.bookmarks.getItemIndex(this._id);
    PlacesUtils.bookmarks.moveItem(this._id, this._newContainer, this._newIndex);
    this._undoIndex = PlacesUtils.bookmarks.getItemIndex(this._id);
  },

  undoTransaction: function MITXN_undoTransaction()
  {
    
    
    if (this._newContainer == this._oldContainer &&
        this._oldIndex > this._undoIndex)
      PlacesUtils.bookmarks.moveItem(this._id, this._oldContainer, this._oldIndex + 1);
    else
      PlacesUtils.bookmarks.moveItem(this._id, this._oldContainer, this._oldIndex);
  }
};










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

  
  
  if (PlacesUtils.itemIsLivemark(aItemId))
    return new PlacesRemoveLivemarkTransaction(aItemId);

  this._id = aItemId;
  this._itemType = PlacesUtils.bookmarks.getItemType(this._id);
  if (this._itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
    this.childTransactions = this._getFolderContentsTransactions();
    
    let txn = PlacesUtils.bookmarks.getRemoveFolderTransaction(this._id);
    this.childTransactions.push(txn);
  }
  else if (this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
    this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._id);
    this._keyword = PlacesUtils.bookmarks.getKeywordForBookmark(this._id);
  }

  if (this._itemType != Ci.nsINavBookmarksService.TYPE_SEPARATOR)
    this._title = PlacesUtils.bookmarks.getItemTitle(this._id);

  this._oldContainer = PlacesUtils.bookmarks.getFolderIdForItem(this._id);
  this._annotations = PlacesUtils.getAnnotationsForItem(this._id);
  this._dateAdded = PlacesUtils.bookmarks.getItemDateAdded(this._id);
  this._lastModified = PlacesUtils.bookmarks.getItemLastModified(this._id);
}

PlacesRemoveItemTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function RITXN_doTransaction()
  {
    this._oldIndex = PlacesUtils.bookmarks.getItemIndex(this._id);

    if (this._itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      let txn = new PlacesAggregatedTransaction("Remove item childTxn",
                                                this.childTransactions);
      txn.doTransaction();
    }
    else {
      
      let tags = this._uri ? PlacesUtils.tagging.getTagsForURI(this._uri) : null;

      PlacesUtils.bookmarks.removeItem(this._id);

      
      
      if (tags && PlacesUtils.getMostRecentBookmarkForURI(this._uri) == -1) {
        this._tags = tags;
      }
    }
  },

  undoTransaction: function RITXN_undoTransaction()
  {
    if (this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
      this._id = PlacesUtils.bookmarks.insertBookmark(this._oldContainer,
                                                      this._uri,
                                                      this._oldIndex,
                                                      this._title);
      if (this._tags && this._tags.length > 0)
        PlacesUtils.tagging.tagURI(this._uri, this._tags);
      if (this._keyword)
        PlacesUtils.bookmarks.setKeywordForBookmark(this._id, this._keyword);
    }
    else if (this._itemType == Ci.nsINavBookmarksService.TYPE_FOLDER) {
      let txn = new PlacesAggregatedTransaction("Remove item childTxn",
                                                this.childTransactions);
      txn.undoTransaction();
    }
    else 
      this._id = PlacesUtils.bookmarks.insertSeparator(this._oldContainer, this._oldIndex);

    if (this._annotations.length > 0)
      PlacesUtils.setAnnotationsForItem(this._id, this._annotations);

    PlacesUtils.bookmarks.setItemDateAdded(this._id, this._dateAdded);
    PlacesUtils.bookmarks.setItemLastModified(this._id, this._lastModified);
  },

  



  _getFolderContentsTransactions:
  function RITXN__getFolderContentsTransactions()
  {
    let transactions = [];
    let contents =
      PlacesUtils.getFolderContents(this._id, false, false).root;
    for (let i = 0; i < contents.childCount; ++i) {
      let txn = new PlacesRemoveItemTransaction(contents.getChild(i).itemId);
      transactions.push(txn);
    }
    contents.containerOpen = false;
    
    return transactions.reverse();
  }
};












function PlacesEditItemTitleTransaction(id, newTitle)
{
  this._id = id;
  this._newTitle = newTitle;
  this._oldTitle = "";
}

PlacesEditItemTitleTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EITTXN_doTransaction()
  {
    this._oldTitle = PlacesUtils.bookmarks.getItemTitle(this._id);
    PlacesUtils.bookmarks.setItemTitle(this._id, this._newTitle);
  },

  undoTransaction: function EITTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemTitle(this._id, this._oldTitle);
  }
};












function PlacesEditBookmarkURITransaction(aBookmarkId, aNewURI) {
  this._id = aBookmarkId;
  this._newURI = aNewURI;
}

PlacesEditBookmarkURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBUTXN_doTransaction()
  {
    this._oldURI = PlacesUtils.bookmarks.getBookmarkURI(this._id);
    PlacesUtils.bookmarks.changeBookmarkURI(this._id, this._newURI);
    
    this._tags = PlacesUtils.tagging.getTagsForURI(this._oldURI);
    if (this._tags.length != 0) {
      
      if (PlacesUtils.getBookmarksForURI(this._oldURI, {}).length == 0)
        PlacesUtils.tagging.untagURI(this._oldURI, this._tags);
      PlacesUtils.tagging.tagURI(this._newURI, this._tags);
    }
  },

  undoTransaction: function EBUTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.changeBookmarkURI(this._id, this._oldURI);
    
    if (this._tags.length != 0) {
      
      if (PlacesUtils.getBookmarksForURI(this._newURI, {}).length == 0)
        PlacesUtils.tagging.untagURI(this._newURI, this._tags);
      PlacesUtils.tagging.tagURI(this._oldURI, this._tags);
    }
  }
};















function PlacesSetItemAnnotationTransaction(aItemId, aAnnotationObject)
{
  this.id = aItemId;
  this._anno = aAnnotationObject;
  
  this._oldAnno = { name: this._anno.name,
                    type: Ci.nsIAnnotationService.TYPE_STRING,
                    flags: 0,
                    value: null,
                    expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
}

PlacesSetItemAnnotationTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SIATXN_doTransaction()
  {
    
    
    if (PlacesUtils.annotations.itemHasAnnotation(this.id, this._anno.name)) {
      
      let flags = {}, expires = {}, mimeType = {}, type = {};
      PlacesUtils.annotations.getItemAnnotationInfo(this.id, this._anno.name,
                                                    flags, expires, mimeType,
                                                    type);
      this._oldAnno.flags = flags.value;
      this._oldAnno.expires = expires.value;
      this._oldAnno.mimeType = mimeType.value;
      this._oldAnno.type = type.value;
      this._oldAnno.value = PlacesUtils.annotations
                                       .getItemAnnotation(this.id,
                                                          this._anno.name);
    }

    PlacesUtils.setAnnotationsForItem(this.id, [this._anno]);
  },

  undoTransaction: function SIATXN_undoTransaction()
  {
    PlacesUtils.setAnnotationsForItem(this.id, [this._oldAnno]);
  }
};















function PlacesSetPageAnnotationTransaction(aURI, aAnnotationObject)
{
  this._uri = aURI;
  this._anno = aAnnotationObject;
  
  this._oldAnno = { name: this._anno.name,
                    type: Ci.nsIAnnotationService.TYPE_STRING,
                    flags: 0,
                    value: null,
                    expires: Ci.nsIAnnotationService.EXPIRE_NEVER };

  if (PlacesUtils.annotations.pageHasAnnotation(this._uri, this._anno.name)) {
    
    let flags = {}, expires = {}, mimeType = {}, type = {};
    PlacesUtils.annotations.getPageAnnotationInfo(this._uri, this._anno.name,
                                                  flags, expires, mimeType, type);
    this._oldAnno.flags = flags.value;
    this._oldAnno.expires = expires.value;
    this._oldAnno.mimeType = mimeType.value;
    this._oldAnno.type = type.value;
    this._oldAnno.value = PlacesUtils.annotations
                                     .getPageAnnotation(this._uri, this._anno.name);
  }

}

PlacesSetPageAnnotationTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SPATXN_doTransaction()
  {
    PlacesUtils.setAnnotationsForURI(this._uri, [this._anno]);
  },

  undoTransaction: function SPATXN_undoTransaction()
  {
    PlacesUtils.setAnnotationsForURI(this._uri, [this._oldAnno]);
  }
};












function PlacesEditBookmarkKeywordTransaction(id, newKeyword) {
  this.id = id;
  this._newKeyword = newKeyword;
  this._oldKeyword = "";

}

PlacesEditBookmarkKeywordTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBKTXN_doTransaction()
  {
    this._oldKeyword = PlacesUtils.bookmarks.getKeywordForBookmark(this.id);
    PlacesUtils.bookmarks.setKeywordForBookmark(this.id, this._newKeyword);
  },

  undoTransaction: function EBKTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setKeywordForBookmark(this.id, this._oldKeyword);
  }
};












function PlacesEditBookmarkPostDataTransaction(aItemId, aPostData)
{
  this.id = aItemId;
  this._newPostData = aPostData;
  this._oldPostData = null;
}

PlacesEditBookmarkPostDataTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function EBPDTXN_doTransaction()
  {
    this._oldPostData = PlacesUtils.getPostDataForBookmark(this.id);
    PlacesUtils.setPostDataForBookmark(this.id, this._newPostData);
  },

  undoTransaction: function EBPDTXN_undoTransaction()
  {
    PlacesUtils.setPostDataForBookmark(this.id, this._oldPostData);
  }
};












function PlacesEditLivemarkSiteURITransaction(folderId, uri)
{
  this._folderId = folderId;
  this._newURI = uri;
  this._oldURI = null;
}

PlacesEditLivemarkSiteURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function ELSUTXN_doTransaction()
  {
    this._oldURI = PlacesUtils.livemarks.getSiteURI(this._folderId);
    PlacesUtils.livemarks.setSiteURI(this._folderId, this._newURI);
  },

  undoTransaction: function ELSUTXN_undoTransaction()
  {
    PlacesUtils.livemarks.setSiteURI(this._folderId, this._oldURI);
  }
};












function PlacesEditLivemarkFeedURITransaction(folderId, uri)
{
  this._folderId = folderId;
  this._newURI = uri;
  this._oldURI = null;
}

PlacesEditLivemarkFeedURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function ELFUTXN_doTransaction()
  {
    this._oldURI = PlacesUtils.livemarks.getFeedURI(this._folderId);
    PlacesUtils.livemarks.setFeedURI(this._folderId, this._newURI);
    PlacesUtils.livemarks.reloadLivemarkFolder(this._folderId);
  },

  undoTransaction: function ELFUTXN_undoTransaction()
  {
    PlacesUtils.livemarks.setFeedURI(this._folderId, this._oldURI);
    PlacesUtils.livemarks.reloadLivemarkFolder(this._folderId);
  }
};












function PlacesEditItemDateAddedTransaction(id, newDateAdded)
{
  this.id = id;
  this._newDateAdded = newDateAdded;
  this._oldDateAdded = null;
}

PlacesEditItemDateAddedTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this.id,
  set container(val) this.id = val,

  doTransaction: function EIDATXN_doTransaction()
  {
    this._oldDateAdded = PlacesUtils.bookmarks.getItemDateAdded(this.id);
    PlacesUtils.bookmarks.setItemDateAdded(this.id, this._newDateAdded);
  },

  undoTransaction: function EIDATXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemDateAdded(this.id, this._oldDateAdded);
  }
};












function PlacesEditItemLastModifiedTransaction(id, newLastModified)
{
  this.id = id;
  this._newLastModified = newLastModified;
  this._oldLastModified = null;
}

PlacesEditItemLastModifiedTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  
  get container() this.id,
  set container(val) this.id = val,

  doTransaction:
  function EILMTXN_doTransaction()
  {
    this._oldLastModified = PlacesUtils.bookmarks.getItemLastModified(this.id);
    PlacesUtils.bookmarks.setItemLastModified(this.id, this._newLastModified);
  },

  undoTransaction:
  function EILMTXN_undoTransaction()
  {
    PlacesUtils.bookmarks.setItemLastModified(this.id, this._oldLastModified);
  }
};










function PlacesSortFolderByNameTransaction(aFolderId)
{
  this._folderId = aFolderId;
  this._oldOrder = null;
}

PlacesSortFolderByNameTransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function SFBNTXN_doTransaction()
  {
    this._oldOrder = [];

    let contents =
      PlacesUtils.getFolderContents(this._folderId, false, false).root;
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
          preSep.splice(0);
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














function PlacesTagURITransaction(aURI, aTags)
{
  this._uri = aURI;
  this._tags = aTags;
  this._unfiledItemId = -1;
}

PlacesTagURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function TUTXN_doTransaction()
  {
    if (PlacesUtils.getMostRecentBookmarkForURI(this._uri) == -1) {
      
      this._unfiledItemId =
        PlacesUtils.bookmarks
                   .insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                   this._uri,
                                   PlacesUtils.bookmarks.DEFAULT_INDEX,
                                   PlacesUtils.history.getPageTitle(this._uri));
      if (this._GUID)
        PlacesUtils.bookmarks.setItemGUID(this._unfiledItemId, this._GUID);
    }
    PlacesUtils.tagging.tagURI(this._uri, this._tags);
  },

  undoTransaction: function TUTXN_undoTransaction()
  {
    if (this._unfiledItemId != -1) {
      
      if (PlacesUtils.annotations.itemHasAnnotation(this._unfiledItemId, PlacesUtils.GUID_ANNO)) {
        this._GUID = PlacesUtils.bookmarks.getItemGUID(this._unfiledItemId);
      }
      PlacesUtils.bookmarks.removeItem(this._unfiledItemId);
      this._unfiledItemId = -1;
    }
    PlacesUtils.tagging.untagURI(this._uri, this._tags);
  }
};














function PlacesUntagURITransaction(aURI, aTags)
{
  this._uri = aURI;
  if (aTags) {    
    
    
    
    this._tags = aTags;
    for (let i = 0; i < aTags.length; ++i) {
      if (typeof(this._tags[i]) == "number")
        this._tags[i] = PlacesUtils.bookmarks.getItemTitle(this._tags[i]);
    }
  }
  else {
    this._tags = PlacesUtils.tagging.getTagsForURI(this._uri);
  }
}

PlacesUntagURITransaction.prototype = {
  __proto__: BaseTransaction.prototype,

  doTransaction: function UTUTXN_doTransaction()
  {
    PlacesUtils.tagging.untagURI(this._uri, this._tags);
  },

  undoTransaction: function UTUTXN_undoTransaction()
  {
    PlacesUtils.tagging.tagURI(this._uri, this._tags);
  }
};
