






































function LOG(str) {
  dump("*** " + str + "\n");
}

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";
const POST_DATA_ANNO = "URIProperties/POSTData";

function QI_node(aNode, aIID) {
  var result = null;
  try {
    result = aNode.QueryInterface(aIID);
  }
  catch (e) {
  }
  NS_ASSERT(result, "Node QI Failed");
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

  


  _bookmarks: null,
  get bookmarks() {
    if (!this._bookmarks) {
      this._bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                        getService(Ci.nsINavBookmarksService);
    }
    return this._bookmarks;
  },

  


  _history: null,
  get history() {
    if (!this._history) {
      this._history = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsINavHistoryService);
    }
    return this._history;
  },

  


  _livemarks: null,
  get livemarks() {
    if (!this._livemarks) {
      this._livemarks = Cc["@mozilla.org/browser/livemark-service;2"].
                        getService(Ci.nsILivemarkService);
    }
    return this._livemarks;
  },

  


  _annotations: null,
  get annotations() {
    if (!this._annotations) {
      this._annotations = Cc["@mozilla.org/browser/annotation-service;1"].
                          getService(Ci.nsIAnnotationService);
    }
    return this._annotations;
  },

  


  _favicons: null,
  get favicons() {
    if (!this._favicons) {
      this._favicons = Cc["@mozilla.org/browser/favicon-service;1"].
                       getService(Ci.nsIFaviconService);
    }
    return this._favicons;
  },

  _RDF: null,
  get RDF() {
    if (!this._RDF)
      this._RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                  getService(Ci.nsIRDFService);
    return this._RDF;
  },

  _localStore: null,
  get localStore() {
    if (!this._localStore)
      this._localStore = this.RDF.GetDataSource("rdf:local-store");
    return this._localStore;
  },

  


  _tm: null,
  get tm() {
    if (!this._tm) {
      this._tm = Cc["@mozilla.org/transactionmanager;1"].
                 createInstance(Ci.nsITransactionManager);
    }
    return this._tm;
  },

  





  _uri: function PU__uri(aSpec) {
    NS_ASSERT(aSpec, "empty URL spec");
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    return ios.newURI(aSpec, null, null);
  },

  





  _wrapString: function PU__wrapString(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  


  __bundle: null,
  get _bundle() {
    if (!this.__bundle) {
      const PLACES_STRING_BUNDLE_URI =
        "chrome://browser/locale/places/places.properties";
      this.__bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                      getService(Ci.nsIStringBundleService).
                      createBundle(PLACES_STRING_BUNDLE_URI);
    }
    return this.__bundle;
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PU_getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  





  nodeIsFolder: function PU_nodeIsFolder(aNode) {
    NS_ASSERT(aNode, "null node");
    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER);
  },

  





  nodeIsBookmark: function PU_nodeIsBookmark(aNode) {
    NS_ASSERT(aNode, "null node");
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI &&
           aNode.itemId != -1;
  },

  





  nodeIsSeparator: function PU_nodeIsSeparator(aNode) {
    NS_ASSERT(aNode, "null node");

    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR);
  },

  





  nodeIsVisit: function PU_nodeIsVisit(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_VISIT ||
           type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  





  nodeIsURI: function PU_nodeIsURI(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_URI ||
           type == NHRN.RESULT_TYPE_VISIT ||
           type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  





  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  






  nodeIsReadOnly: function PU_nodeIsReadOnly(aNode) {
    NS_ASSERT(aNode, "null node");

    if (this.nodeIsFolder(aNode))
      return this.bookmarks.getFolderReadonly(aNode.itemId);
    if (this.nodeIsQuery(aNode))
      return asQuery(aNode).childrenReadOnly;
    return false;
  },

  





  nodeIsHost: function PU_nodeIsHost(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_HOST;
  },

  





  nodeIsContainer: function PU_nodeIsContainer(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_HOST ||
           type == NHRN.RESULT_TYPE_QUERY ||
           type == NHRN.RESULT_TYPE_FOLDER ||
           type == NHRN.RESULT_TYPE_DAY ||
           type == NHRN.RESULT_TYPE_REMOTE_CONTAINER;
  },

  













  nodeIsRemoteContainer: function PU_nodeIsRemoteContainer(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    if (aNode.type == NHRN.RESULT_TYPE_REMOTE_CONTAINER)
      return true;
    if (this.nodeIsFolder(aNode))
      return asContainer(aNode).remoteContainerType != "";
    return false;
  },

 






  nodeIsLivemarkContainer: function PU_nodeIsLivemarkContainer(aNode) {
    return (this.nodeIsRemoteContainer(aNode) &&
            asContainer(aNode).remoteContainerType ==
               "@mozilla.org/browser/livemark-service;2");
  },

 





  nodeIsLivemarkItem: function PU_nodeIsLivemarkItem(aNode) {
    if (this.nodeIsBookmark(aNode)) {
      if (this.annotations
              .itemHasAnnotation(aNode.itemId, "livemark/bookmarkFeedURI"))
        return true;
    }

    return false;
  },

  





  isReadonlyFolder: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return this.nodeIsFolder(aNode) &&
           this.bookmarks.getFolderReadonly(aNode.itemId);
  },

  






  getIndexOfNode: function PU_getIndexOfNode(aNode) {
    NS_ASSERT(aNode, "null node");

    var parent = aNode.parent;
    if (!parent || !PlacesUtils.nodeIsContainer(parent))
      return -1;
    var wasOpen = parent.containerOpen;
    parent.containerOpen = true;
    var cc = parent.childCount;
    asContainer(parent);
    for (var i = 0; i < cc && parent.getChild(i) != aNode; ++i);
    parent.containerOpen = wasOpen;
    return i < cc ? i : -1;
  },

  












  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI) {
    switch (aType) {
    case this.TYPE_X_MOZ_PLACE_CONTAINER:
    case this.TYPE_X_MOZ_PLACE:
    case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      
      
      
      
      
      var wrapped = "";
      if (aNode.itemId != -1) 
        wrapped += aNode.itemId + NEWLINE;
      else
        wrapped += "0" + NEWLINE;

      if (this.nodeIsURI(aNode) || this.nodeIsQuery(aNode))
        wrapped += aNode.uri + NEWLINE;
      else
        wrapped += NEWLINE;

      if (this.nodeIsFolder(aNode.parent))
        wrapped += aNode.parent.itemId + NEWLINE;
      else
        wrapped += "0" + NEWLINE;

      wrapped += this.getIndexOfNode(aNode);
      return wrapped;
    case this.TYPE_X_MOZ_URL:
      return (aOverrideURI || aNode.uri) + NEWLINE + aNode.title;
    case this.TYPE_HTML:
      return "<A HREF=\"" + (aOverrideURI || aNode.uri) + "\">" +
             aNode.title + "</A>";
    }
    
    return (aOverrideURI || aNode.uri);
  },

  










  _getURIItemCopyTransaction: function (aURI, aContainer, aIndex) {
    var title = this.history.getPageTitle(aURI);
    return new PlacesCreateItemTransaction(aURI, aContainer, aIndex, title);
  },

  













  _getBookmarkItemCopyTransaction:
  function PU__getBookmarkItemCopyTransaction(aId, aContainer, aIndex,
                                              aExcludeAnnotations) {
    var bookmarks = this.bookmarks;
    var itemURL = bookmarks.getBookmarkURI(aId);
    var itemTitle = bookmarks.getItemTitle(aId);
    var keyword = bookmarks.getKeywordForBookmark(aId);
    var annos = this.getAnnotationsForItem(aId);
    if (aExcludeAnnotations) {
      annos =
        annos.filter(function(aValue, aIndex, aArray) {
                       return aExcludeAnnotations.indexOf(aValue.name) == -1;
                    });
    }
    var createTxn =
      new PlacesCreateItemTransaction(itemURL, aContainer, aIndex, itemTitle,
                                      keyword, annos);
    return createTxn;
  },

  










  _getFolderCopyTransaction:
  function PU__getFolderCopyTransaction(aData, aContainer, aIndex) {
    var self = this;
    function getChildItemsTransactions(aFolderId) {
      var childItemsTransactions = [];
      var children = self.getFolderContents(aFolderId, false, false);
      var cc = children.childCount;
      for (var i = 0; i < cc; ++i) {
        var txn = null;
        var node = children.getChild(i);
        if (self.nodeIsFolder(node)) {
          var nodeFolderId = node.itemId;
          var title = self.bookmarks.getItemTitle(nodeFolderId);
          var annos = self.getAnnotationsForItem(nodeFolderId);
          var folderItemsTransactions =
            getChildItemsTransactions(nodeFolderId);
          txn = new PlacesCreateFolderTransaction(title, -1, aIndex, annos,
                                                  folderItemsTransactions);
        }
        else if (self.nodeIsBookmark(node)) {
          txn = self._getBookmarkItemCopyTransaction(node.itemId, -1,
                                                     aIndex);
        }
        else if (self.nodeIsURI(node) || self.nodeIsQuery(node)) {
          
          txn = self._getURIItemCopyTransaction(self._uri(node.uri), -1,
                                                aIndex);
        }
        else if (self.nodeIsSeparator(node))
          txn = new PlacesCreateSeparatorTransaction(-1, aIndex);

        NS_ASSERT(txn, "Unexpected item under a bookmarks folder");
        if (txn)
          childItemsTransactions.push(txn);
      }
      return childItemsTransactions;
    }

    var title = this.bookmarks.getItemTitle(aData.id);
    var annos = this.getAnnotationsForItem(aData.id);
    var createTxn =
      new PlacesCreateFolderTransaction(title, aContainer, aIndex, annos,
                                        getChildItemsTransactions(aData.id));
    return createTxn;
  },

  








  unwrapNodes: function PU_unwrapNodes(blob, type) {
    
    var parts = blob.split("\n");
    var nodes = [];
    for (var i = 0; i < parts.length; ++i) {
      var data = { };
      switch (type) {
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
        
        
        if (i > (parts.length - 4))
          break;
        nodes.push({  id: parseInt(parts[i++]),
                      uri: parts[i] ? this._uri(parts[i]) : null,
                      parent: parseInt(parts[++i]),
                      index: parseInt(parts[++i]) });
        break;
      case this.TYPE_X_MOZ_URL:
        
        if (i > (parts.length - 2))
          break;
        nodes.push({  uri: this._uri(parts[i++]),
                      title: parts[i] ? parts[i] : parts[i-1] });
        break;
      case this.TYPE_UNICODE:
        
        if (i > (parts.length - 1))
          break;
        nodes.push({  uri: this._uri(parts[i]) });
        break;
      default:
        LOG("Cannot unwrap data of type " + type);
        throw Cr.NS_ERROR_INVALID_ARG;
      }
    }
    return nodes;
  },

  















  makeTransaction: function PU_makeTransaction(data, type, container,
                                               index, copy) {
    switch (type) {
    case this.TYPE_X_MOZ_PLACE_CONTAINER:
      if (data.id > 0) {
        
        if (copy)
          return this._getFolderCopyTransaction(data, container, index);
      }
      break;
    case this.TYPE_X_MOZ_PLACE:
      if (data.id <= 0)
        return this._getURIItemCopyTransaction(data.uri, container, index);
  
      if (copy) {
        
        
        var copyBookmarkAnno = 
          this._getBookmarkItemCopyTransaction(data.id, container, index,
                                               ["livemark/bookmarkFeedURI"]);
        return copyBookmarkAnno;
      }
      break;
    case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      if (copy) {
        
        
        return new PlacesCreateSeparatorTransaction(container, index);
      }
      break;
    case this.TYPE_X_MOZ_URL:
    case this.TYPE_UNICODE:
      var title = type == this.TYPE_X_MOZ_URL ? data.title : data.uri;
      var createTxn =
        new PlacesCreateItemTransaction(data.uri, container, index, title);
      return createTxn;
    default:
      return null;
    }
    if (data.id <= 0)
      return null;

    
    return new PlacesMoveItemTransaction(data.id, container, index);
  },

  













  getFolderContents:
  function PU_getFolderContents(aFolderId, aExcludeItems, aExpandQueries) {
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    var options = this.history.getNewQueryOptions();
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    options.excludeItems = aExcludeItems;
    options.expandQueries = aExpandQueries;

    var result = this.history.executeQuery(query, options);
    result.root.containerOpen = true;
    return asContainer(result.root);
  },

  








  
































  showAddBookmarkUI: function PU_showAddBookmarkUI(aURI,
                                                   aTitle,
                                                   aDescription,
                                                   aDefaultInsertionPoint,
                                                   aShowPicker,
                                                   aLoadInSidebar,
                                                   aKeyword,
                                                   aPostData) {
    var info = {
      action: "add",
      type: "bookmark"
    };

    if (aURI)
      info.uri = aURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows = ["folder picker"];
    }

    if (aLoadInSidebar)
      info.loadBookmarkInSidebar = true;

    if (typeof(aKeyword) == "string") {
      info.keyword = aKeyword;
      if (typeof(aPostData) == "string")
        info.postData = aPostData;
    }

    return this._showBookmarkDialog(info);
  },

  










  showMinimalAddBookmarkUI:
  function PU_showMinimalAddBookmarkUI(aURI, aTitle, aDescription,
                                       aDefaultInsertionPoint, aShowPicker,
                                       aLoadInSidebar, aKeyword, aPostData) {
    var info = {
      action: "add",
      type: "bookmark",
      hiddenRows: ["location", "description", "load in sidebar"]
    };
    if (aURI)
      info.uri = aURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folder picker");
    }

    if (aLoadInSidebar)
      info.loadBookmarkInSidebar = true;

    if (typeof(aKeyword) == "string") {
      info.keyword = aKeyword;
      if (typeof(aPostData) == "string")
        info.postData = aPostData;
    }
    else
      info.hiddenRows.push("keyword");

    this._showBookmarkDialog(info, true);
  },

  





















  showAddLivemarkUI: function PU_showAddLivemarkURI(aFeedURI,
                                                    aSiteURI,
                                                    aTitle,
                                                    aDescription,
                                                    aDefaultInsertionPoint,
                                                    aShowPicker) {
    var info = {
      action: "add",
      type: "livemark"
    };

    if (aFeedURI)
      info.feedURI = aFeedURI;
    if (aSiteURI)
      info.siteURI = aSiteURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows = ["folder picker"];
    }
    return this._showBookmarkDialog(info);
  },

  







  showMinimalAddLivemarkUI:
  function PU_showMinimalAddLivemarkURI(aFeedURI, aSiteURI, aTitle,
                                        aDescription, aDefaultInsertionPoint,
                                        aShowPicker) {
    var info = {
      action: "add",
      type: "livemark",
      hiddenRows: ["feedURI", "siteURI", "description"]
    };

    if (aFeedURI)
      info.feedURI = aFeedURI;
    if (aSiteURI)
      info.siteURI = aSiteURI;

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDescription)
      info.description = aDescription;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folder picker");
    }
    this._showBookmarkDialog(info, true);
  },

  








  showMinimalAddMultiBookmarkUI: function PU_showAddMultiBookmarkUI(aURIList) {
    NS_ASSERT(aURIList.length,
              "showAddMultiBookmarkUI expects a list of nsIURI objects");
    var info = {
      action: "add",
      type: "folder",
      hiddenRows: ["description"],
      URIList: aURIList
    };
    this._showBookmarkDialog(info, true);
  },

  






  showBookmarkProperties: function PU_showBookmarkProperties(aId) {
    var info = {
      action: "edit",
      type: "bookmark",
      bookmarkId: aId
    };
    return this._showBookmarkDialog(info);
  },

  






  showFolderProperties: function PU_showFolderProperties(aId) {
    var info = {
      action: "edit",
      type: "folder",
      folderId: aId
    };
    return this._showBookmarkDialog(info);
  },

  













  showAddFolderUI:
  function PU_showAddFolderUI(aTitle, aDefaultInsertionPoint, aShowPicker) {
    var info = {
      action: "add",
      type: "folder",
      hiddenRows: []
    };

    
    if (typeof(aTitle) == "string")
      info.title = aTitle;

    if (aDefaultInsertionPoint) {
      info.defaultInsertionPoint = aDefaultInsertionPoint;
      if (!aShowPicker)
        info.hiddenRows.push("folder picker");
    }
    return this._showBookmarkDialog(info);
  },

  















  _showBookmarkDialog: function PU__showBookmarkDialog(aInfo, aMinimalUI) {
    var dialogURL = aMinimalUI ?
                    "chrome://browser/content/places/bookmarkPageDialog.xul" :
                    "chrome://browser/content/places/bookmarkProperties.xul";

    var features;
    if (aMinimalUI)
#ifdef XP_MACOSX
      features = "centerscreen,chrome,dialog,resizable,modal";
#else
      features = "centerscreen,chrome,dialog,resizable,dependent";
#endif
    else
      features = "centerscreen,chrome,modal,resizable=no";
    window.openDialog(dialogURL, "",  features, aInfo);
    return ("performed" in aInfo && aInfo.performed);
  },

  





  getViewForNode: function(aNode) {
    var node = aNode;
    while (node) {
      
      if (node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  







  checkURLSecurity: function PU_checkURLSecurity(aURINode) {
    if (!this.nodeIsBookmark(aURINode)) {
      var uri = this._uri(aURINode.uri);
      if (uri.schemeIs("javascript") || uri.schemeIs("data")) {
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        var errorStr = this.getString("load-js-data-url-error");
        promptService.alert(window, brandShortName, errorStr);
        return false;
      }
    }
    return true;
  },

  







  getAnnotationsForURI: function PU_getAnnotationsForURI(aURI) {
    var annosvc = this.annotations;
    var annos = [], val = null;
    var annoNames = annosvc.getPageAnnotationNames(aURI, {});
    for (var i = 0; i < annoNames.length; i++) {
      var flags = {}, exp = {}, mimeType = {}, storageType = {};
      annosvc.getPageAnnotationInfo(aURI, annoNames[i], flags, exp, mimeType, storageType);
      switch (storageType.value) {
        case annosvc.TYPE_INT32:
          val = annosvc.getPageAnnotationInt32(aURI, annoNames[i]);
          break;
        case annosvc.TYPE_INT64:
          val = annosvc.getPageAnnotationInt64(aURI, annoNames[i]);
          break;
        case annosvc.TYPE_DOUBLE:
          val = annosvc.getPageAnnotationDouble(aURI, annoNames[i]);
          break;
        case annosvc.TYPE_STRING:
          val = annosvc.getPageAnnotationString(aURI, annoNames[i]);
          break;
        case annosvc.TYPE_BINARY:
          var data = {}, length = {}, mimeType = {};
          annosvc.getPageAnnotationBinary(aURI, annoNames[i], data, length, mimeType);
          val = data.value;
          break;
      }
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
      annosvc.getItemAnnotationInfo(aItemId, annoNames[i], flags, exp,
                                    mimeType, storageType);
      switch (storageType.value) {
        case annosvc.TYPE_INT32:
          val = annosvc.getItemAnnotationInt32(aItemId, annoNames[i]);
          break;
        case annosvc.TYPE_INT64:
          val = annosvc.getItemAnnotationInt64(aItemId, annoNames[i]);
          break;
        case annosvc.TYPE_DOUBLE:
          val = annosvc.getItemAnnotationDouble(aItemId, annoNames[i]);
          break;
        case annosvc.TYPE_STRING:
          val = annosvc.getItemAnnotationString(aItemId, annoNames[i]);
          break;
        case annosvc.TYPE_BINARY:
          var data = {}, length = {}, mimeType = {};
          annosvc.getItemAnnotationBinary(aItemId, annoNames[i], data, length, mimeType);
          val = data.value;
          break;
      }
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
      switch (anno.type) {
        case annosvc.TYPE_INT32:
          annosvc.setPageAnnotationInt32(aURI, anno.name, anno.value,
                                         anno.flags, anno.expires);
          break;
        case annosvc.TYPE_INT64:
          annosvc.setPageAnnotationInt64(aURI, anno.name, anno.value,
                                         anno.flags, anno.expires);
          break;
        case annosvc.TYPE_DOUBLE:
          annosvc.setPageAnnotationDouble(aURI, anno.name, anno.value,
                                          anno.flags, anno.expires);
          break;
        case annosvc.TYPE_STRING:
          annosvc.setPageAnnotationString(aURI, anno.name, anno.value,
                                          anno.flags, anno.expires);
          break;
        case annosvc.TYPE_BINARY:
          annosvc.setPageAnnotationBinary(aURI, anno.name, anno.value,
                                          anno.value.length, anno.mimeType,
                                          anno.flags, anno.expires);
          break;
      }
    });
  },

  








  setAnnotationsForItem: function PU_setAnnotationsForItem(aItemId, aAnnos) {
    var annosvc = this.annotations;
    aAnnos.forEach(function(anno) {
      switch (anno.type) {
        case annosvc.TYPE_INT32:
          annosvc.setItemAnnotationInt32(aItemId, anno.name, anno.value,
                                         anno.flags, anno.expires);
          break;
        case annosvc.TYPE_INT64:
          annosvc.setItemAnnotationInt64(aItemId, anno.name, anno.value,
                                         anno.flags, anno.expires);
          break;
        case annosvc.TYPE_DOUBLE:
          annosvc.setItemAnnotationDouble(aItemId, anno.name, anno.value,
                                          anno.flags, anno.expires);
          break;
        case annosvc.TYPE_STRING:
          annosvc.setItemAnnotationString(aItemId, anno.name, anno.value,
                                          anno.flags, anno.expires);
          break;
        case annosvc.TYPE_BINARY:
          annosvc.setItemAnnotationBinary(aItemId, anno.name, anno.value,
                                          anno.value.length, anno.mimeType,
                                          anno.flags, anno.expires);
          break;
      }
    });
  },

  




  getQueryStringForFolder: function PU_getQueryStringForFolder(aFolderId) {
    var options = this.history.getNewQueryOptions();
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    return this.history.queriesToQueryString([query], 1, options);
  },

  







  getDescriptionFromDocument: function PU_getDescriptionFromDocument(doc) {
    var metaElements = doc.getElementsByTagName("META");
    for (var i = 0; i < metaElements.length; ++i) {
      if (metaElements[i].localName.toLowerCase() == "description" ||
          metaElements[i].httpEquiv.toLowerCase() == "description") {
        return metaElements[i].content;
      }
    }
    return "";
  },

  
  get placesRootId() {
    if (!("_placesRootId" in this))
      this._placesRootId = this.bookmarks.placesRoot;

    return this._placesRootId;
  },

  get bookmarksRootId() {
    if (!("_bookmarksRootId" in this))
      this._bookmarksRootId = this.bookmarks.bookmarksRoot;

    return this._bookmarksRootId;
  },

  get toolbarFolderId() {
    if (!("_toolbarFolderId" in this))
      this._toolbarFolderId = this.bookmarks.toolbarFolder;

    return this._toolbarFolderId;
  },

  





  setPostDataForURI: function PU_setPostDataForURI(aURI, aPostData) {
    const annos = this.annotations;
    if (aPostData)
      annos.setPageAnnotationString(aURI, POST_DATA_ANNO, aPostData, 0, 0);
    else if (annos.pageHasAnnotation(aURI, POST_DATA_ANNO))
      annos.removePageAnnotation(aURI, POST_DATA_ANNO);
  },

  




  getPostDataForURI: function PU_getPostDataForURI(aURI) {
    const annos = this.annotations;
    if (annos.pageHasAnnotation(aURI, POST_DATA_ANNO))
      return annos.getPageAnnotationString(aURI, POST_DATA_ANNO);

    return null;
  }
};

PlacesUtils.GENERIC_VIEW_DROP_TYPES = [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                                       PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                                       PlacesUtils.TYPE_X_MOZ_PLACE,
                                       PlacesUtils.TYPE_X_MOZ_URL];
