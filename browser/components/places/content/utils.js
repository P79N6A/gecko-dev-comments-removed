







































function LOG(str) {
  dump("*** " + str + "\n");
}

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

Components.utils.import("resource://gre/modules/JSON.jsm");

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";
const POST_DATA_ANNO = "bookmarkProperties/POSTData";
const LMANNO_FEEDURI = "livemark/feedURI";
const LMANNO_SITEURI = "livemark/siteURI";
const ORGANIZER_FOLDER_ANNO = "PlacesOrganizer/OrganizerFolder";
const ORGANIZER_QUERY_ANNO = "PlacesOrganizer/OrganizerQuery";

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

  get globalHistory() {
    delete this.globalHistory;
    return this.globalHistory = Cc["@mozilla.org/browser/global-history;2"].
                                getService(Ci.nsIBrowserHistory);
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

  


  get microsummaries() {
    delete this.microsummaries;
    return this.microsummaries = Cc["@mozilla.org/microsummary/service;1"].
                                 getService(Ci.nsIMicrosummaryService);
  },

  


  get tagging() {
    delete this.tagging;
    return this.tagging = Cc["@mozilla.org/browser/tagging-service;1"].
                          getService(Ci.nsITaggingService);
  },

  get RDF() {
    delete this.RDF;
    return this.RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                      getService(Ci.nsIRDFService);
  },

  get localStore() {
    delete this.localStore;
    return this.localStore = this.RDF.GetDataSource("rdf:local-store");
  },

  get ptm() {
    delete this.ptm;
    return this.ptm = Cc["@mozilla.org/browser/placesTransactionsService;1"].
                      getService(Ci.nsIPlacesTransactionsService);
  },

  get clipboard() {
    delete this.clipboard;
    return this.clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                            getService(Ci.nsIClipboard);
  },

  get URIFixup() {
    delete this.URIFixup;
    return this.URIFixup = Cc["@mozilla.org/docshell/urifixup;1"].
                           getService(Ci.nsIURIFixup);
  },

  get ellipsis() {
    delete this.ellipsis;
    var pref = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);
    return this.ellipsis = pref.getComplexValue("intl.ellipsis",
                                                Ci.nsIPrefLocalizedString).data;
  },

  





  _uri: function PU__uri(aSpec) {
    NS_ASSERT(aSpec, "empty URL spec");
    return Cc["@mozilla.org/network/io-service;1"].
           getService(Ci.nsIIOService).
           newURI(aSpec, null, null);
  },

  





  createFixedURI: function PU_createFixedURI(aSpec) {
    return this.URIFixup.createFixupURI(aSpec, 0);
  },

  





  _wrapString: function PU__wrapString(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  


  get _bundle() {
    const PLACES_STRING_BUNDLE_URI =
        "chrome://browser/locale/places/places.properties";
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
    NS_ASSERT(aNode, "null node");
    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER ||
            aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT);
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

  





  uriTypes: [Ci.nsINavHistoryResultNode.RESULT_TYPE_URI,
             Ci.nsINavHistoryResultNode.RESULT_TYPE_VISIT,
             Ci.nsINavHistoryResultNode.RESULT_TYPE_FULL_VISIT],
  nodeIsURI: function PU_nodeIsURI(aNode) {
    NS_ASSERT(aNode, "null node");
    return this.uriTypes.indexOf(aNode.type) != -1;
  },

  





  nodeIsQuery: function PU_nodeIsQuery(aNode) {
    NS_ASSERT(aNode, "null node");
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  






  nodeIsReadOnly: function PU_nodeIsReadOnly(aNode) {
    NS_ASSERT(aNode, "null node");

    if (this.nodeIsFolder(aNode))
      return this.bookmarks.getFolderReadonly(asQuery(aNode).folderItemId);
    if (this.nodeIsQuery(aNode))
      return asQuery(aNode).childrenReadOnly;
    return false;
  },

  





  nodeIsHost: function PU_nodeIsHost(aNode) {
    NS_ASSERT(aNode, "null node");
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY &&
           aNode.parent &&
           asQuery(aNode.parent).queryOptions.resultType ==
             Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY;
  },

  





  nodeIsDay: function PU_nodeIsDay(aNode) {
    NS_ASSERT(aNode, "null node");
    var resultType;
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY &&
           aNode.parent &&
           ((resultType = asQuery(aNode.parent).queryOptions.resultType) ==
               Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY ||
             resultType == Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY);
  },

  





  containerTypes: [Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY,
                   Ci.nsINavHistoryResultNode.RESULT_TYPE_DYNAMIC_CONTAINER],
  nodeIsContainer: function PU_nodeIsContainer(aNode) {
    NS_ASSERT(aNode, "null node");
    return this.containerTypes.indexOf(aNode.type) != -1;
  },

  








  nodeIsDynamicContainer: function PU_nodeIsDynamicContainer(aNode) {
    NS_ASSERT(aNode, "null node");
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
    NS_ASSERT(aNode, "null node");

    return this.nodeIsFolder(aNode) &&
           this.bookmarks.getFolderReadonly(asQuery(aNode).folderItemId);
  },

  



  getConcreteItemId: function PU_getConcreteItemId(aNode) {
    if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT)
      return asQuery(aNode).folderItemId;
    return aNode.itemId;
  },

  






  getIndexOfNode: function PU_getIndexOfNode(aNode) {
    NS_ASSERT(aNode, "null node");

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

  












  wrapNode: function PU_wrapNode(aNode, aType, aOverrideURI) {
    var self = this;

    
    
    
    
    function convertNode(cNode) {
      try {
        if (self.nodeIsFolder(cNode) && cNode.queryOptions.excludeItems)
          return self.getFolderContents(cNode.itemId, false, true).root;
      }
      catch (e) {
      }
      return cNode;
    }

    switch (aType) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        function gatherDataPlace(bNode) {
          var nodeId = 0;
          if (bNode.itemId != -1)
            nodeId = bNode.itemId;
          var nodeUri = bNode.uri
          var nodeTitle = bNode.title;
          var nodeParentId = 0;
          if (bNode.parent && self.nodeIsFolder(bNode.parent))
            nodeParentId = bNode.parent.itemId;
          var nodeIndex = self.getIndexOfNode(bNode);
          var nodeKeyword = self.bookmarks.getKeywordForBookmark(bNode.itemId);
          var nodeAnnos = self.getAnnotationsForItem(bNode.itemId);
          var nodeType = "";
          if (self.nodeIsContainer(bNode))
            nodeType = self.TYPE_X_MOZ_PLACE_CONTAINER;
          else if (self.nodeIsURI(bNode)) 
            nodeType = self.TYPE_X_MOZ_PLACE;
          else if (self.nodeIsSeparator(bNode))
            nodeType = self.TYPE_X_MOZ_PLACE_SEPARATOR;

          var node = { id: nodeId,
                       uri: nodeUri,
                       title: nodeTitle,
                       parent: nodeParentId,
                       index: nodeIndex,
                       keyword: nodeKeyword,
                       annos: nodeAnnos,
                       type: nodeType };

          
          if (self.nodeIsContainer(bNode)) {
            asContainer(bNode);
            if (self.nodeIsLivemarkContainer(bNode)) {
              
              var feedURI = self.livemarks.getFeedURI(bNode.itemId).spec;
              var siteURI = self.livemarks.getSiteURI(bNode.itemId).spec;
              node.uri = { feed: feedURI,
                           site: siteURI };
            }
            else { 
              var wasOpen = bNode.containerOpen;
              if (!wasOpen)
                bNode.containerOpen = true;
              var childNodes = [];
              var cc = bNode.childCount;
              for (var i = 0; i < cc; ++i) {
                var childObj = gatherDataPlace(bNode.getChild(i));
                if (childObj != null)
                  childNodes.push(childObj);
              }
              var parent = node;
              node = { folder: parent,
                       children: childNodes,
                       type: self.TYPE_X_MOZ_PLACE_CONTAINER };
              bNode.containerOpen = wasOpen;
            }
          }
          return node;
        }
        return JSON.toString(gatherDataPlace(convertNode(aNode)));

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
          
          var escapedTitle = htmlEscape(bNode.title);
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

  










  _getURIItemCopyTransaction: function (aData, aContainer, aIndex) {
    return this.ptm.createItem(this._uri(aData.uri), aContainer, aIndex,
                               aData.title, "");
  },

  













  _getBookmarkItemCopyTransaction:
  function PU__getBookmarkItemCopyTransaction(aData, aContainer, aIndex,
                                              aExcludeAnnotations) {
    var itemURL = this._uri(aData.uri);
    var itemTitle = aData.title;
    var keyword = aData.keyword;
    var annos = aData.annos;
    if (aExcludeAnnotations) {
      annos =
        annos.filter(function(aValue, aIndex, aArray) {
                       return aExcludeAnnotations.indexOf(aValue.name) == -1;
                    });
    }
    var childTxns = [];
    if (aData.dateAdded)
      childTxns.push(this.ptm.editItemDateAdded(null, aData.dateAdded));
    if (aData.lastModified)
      childTxns.push(this.ptm.editItemLastModified(null, aData.lastModified));

    return this.ptm.createItem(itemURL, aContainer, aIndex, itemTitle, keyword,
                               annos, childTxns);
  },

  











  _getFolderCopyTransaction:
  function PU__getFolderCopyTransaction(aData, aContainer, aIndex) {
    var self = this;
    function getChildItemsTransactions(aChildren) {
      var childItemsTransactions = [];
      var cc = aChildren.length;
      var index = aIndex;
      for (var i = 0; i < cc; ++i) {
        var txn = null;
        var node = aChildren[i];

        
        
        if (aIndex > -1)
          index = aIndex + i;

        if (node.type == self.TYPE_X_MOZ_PLACE_CONTAINER) {
          if (node.folder) {
            var title = node.folder.title;
            var annos = node.folder.annos;
            var folderItemsTransactions =
              getChildItemsTransactions(node.children);
            txn = self.ptm.createFolder(title, -1, index, annos,
                                        folderItemsTransactions);
          }
          else { 
            var feedURI = self._uri(node.uri.feed);
            var siteURI = self._uri(node.uri.site);
            txn = self.ptm.createLivemark(feedURI, siteURI, node.title,
                                          aContainer, index, node.annos);
          }
        }
        else if (node.type == self.TYPE_X_MOZ_PLACE_SEPARATOR)
          txn = self.ptm.createSeparator(-1, index);
        else if (node.type == self.TYPE_X_MOZ_PLACE)
          txn = self._getBookmarkItemCopyTransaction(node, -1, index);

        NS_ASSERT(txn, "Unexpected item under a bookmarks folder");
        if (txn)
          childItemsTransactions.push(txn);
      }
      return childItemsTransactions;
    }

    var title = aData.folder.title;
    var annos = aData.folder.annos;
    var childItems = getChildItemsTransactions(aData.children);
    if (aData.folder.dateAdded)
      childItems.push(this.ptm.editItemDateAdded(null, aData.folder.dateAdded));
    if (aData.folder.lastModified)
      childItems.push(this.ptm.editItemLastModified(null, aData.folder.lastModified));
    return this.ptm.createFolder(title, aContainer, aIndex, annos, childItems);
  },

  








  unwrapNodes: function PU_unwrapNodes(blob, type) {
    
    var nodes = [];
    switch(type) {
      case this.TYPE_X_MOZ_PLACE:
      case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      case this.TYPE_X_MOZ_PLACE_CONTAINER:
        nodes = JSON.fromString("[" + blob + "]");
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
                         title: titleString ? titleString : uriString });
          }
        }
        break;
      case this.TYPE_UNICODE:
        var parts = blob.split("\n");
        for (var i = 0; i < parts.length; i++) {
          var uriString = parts[i];
          
          if (uriString != "" && this._uri(uriString))
            nodes.push({ uri: uriString, title: uriString });
        }
        break;
      default:
        LOG("Cannot unwrap data of type " + type);
        throw Cr.NS_ERROR_INVALID_ARG;
    }
    return nodes;
  },

  















  makeTransaction: function PU_makeTransaction(data, type, container,
                                               index, copy) {
    switch (data.type) {
    case this.TYPE_X_MOZ_PLACE_CONTAINER:
      if (data.folder) {
        
        if (copy)
          return this._getFolderCopyTransaction(data, container, index);
      }
      else if (copy) {
        
        var feedURI = this._uri(data.uri.feed);
        var siteURI = this._uri(data.uri.site);
        return this.ptm.createLivemark(feedURI, siteURI, data.title, container,
                                       index, data.annos);
      }
      break;
    case this.TYPE_X_MOZ_PLACE:
      if (data.id <= 0)
        return this._getURIItemCopyTransaction(data, container, index);

      if (copy) {
        
        
        var copyBookmarkAnno =
          this._getBookmarkItemCopyTransaction(data, container, index,
                                               ["livemark/bookmarkFeedURI"]);
        return copyBookmarkAnno;
      }
      break;
    case this.TYPE_X_MOZ_PLACE_SEPARATOR:
      if (copy) {
        
        
        return this.ptm.createSeparator(container, index);
      }
      break;
    default:
      if (type == this.TYPE_X_MOZ_URL || type == this.TYPE_UNICODE) {
        var title = (type == this.TYPE_X_MOZ_URL) ? data.title : data.uri;
        return this.ptm.createItem(this._uri(data.uri), container, index,
                                   title);
      }
      return null;
    }
    if (data.id <= 0)
      return null;

    
    var id = data.folder ? data.folder.id : data.id;
    return this.ptm.moveItem(id, container, index);
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
                    "chrome://browser/content/places/bookmarkProperties2.xul" :
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

  





  getViewForNode: function PU_getViewForNode(aNode) {
    var node = aNode;
    while (node) {
      
      if (node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  







  markPageAsTyped: function PU_markPageAsTyped(aURL) {
    this.globalHistory.markPageAsTyped(this.createFixedURI(aURL));
  },

  






  markPageAsFollowedBookmark: function PU_markPageAsFollowedBookmark(aURL) {
    this.history.markPageAsFollowedBookmark(this.createFixedURI(aURL));
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

  




  getQueryStringForFolder: function PU_getQueryStringForFolder(aFolderId) {
    var options = this.history.getNewQueryOptions();
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    return this.history.queriesToQueryString([query], 1, options);
  },

  







  getDescriptionFromDocument: function PU_getDescriptionFromDocument(doc) {
    var metaElements = doc.getElementsByTagName("META");
    for (var i = 0; i < metaElements.length; ++i) {
      if (metaElements[i].name.toLowerCase() == "description" ||
          metaElements[i].httpEquiv.toLowerCase() == "description") {
        return metaElements[i].content;
      }
    }
    return "";
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

  






  getItemDescription: function PU_getItemDescription(aItemId) {
    if (this.annotations.itemHasAnnotation(aItemId, DESCRIPTION_ANNO))
      return this.annotations.getItemAnnotation(aItemId, DESCRIPTION_ANNO);
    return "";
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

  getURLsForContainerNode: function PU_getURLsForContainerNode(aNode) {
    let urls = [];
    if (this.nodeIsFolder(aNode) && asQuery(aNode).queryOptions.excludeItems) {
      
      let contents = this.getFolderContents(aNode.itemId, false, false).root;
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

  


  _confirmOpenInTabs: function PU__confirmOpenInTabs(numTabsToOpen) {
    var pref = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);

    const kWarnOnOpenPref = "browser.tabs.warnOnOpen";
    var reallyOpen = true;
    if (pref.getBoolPref(kWarnOnOpenPref)) {
      if (numTabsToOpen >= pref.getIntPref("browser.tabs.maxOpenBeforeWarn")) {
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        
        var warnOnOpen = { value: true };

        var messageKey = "tabs.openWarningMultipleBranded";
        var openKey = "tabs.openButtonMultiple";
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");

        var buttonPressed = promptService.confirmEx(window,
          this.getString("tabs.openWarningTitle"),
          this.getFormattedString(messageKey, [numTabsToOpen, brandShortName]),
          (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0)
           + (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
          this.getString(openKey), null, null,
          this.getFormattedString("tabs.openWarningPromptMeBranded",
                                  [brandShortName]), warnOnOpen);

        reallyOpen = (buttonPressed == 0);
        
        if (reallyOpen && !warnOnOpen.value)
          pref.setBoolPref(kWarnOnOpenPref, false);
      }
    }
    return reallyOpen;
  },

  


  _openTabset: function PU__openTabset(aItemsToOpen, aEvent) {
    var urls = [];
    for (var i = 0; i < aItemsToOpen.length; i++) {
      var item = aItemsToOpen[i];
      if (item.isBookmark)
        this.markPageAsFollowedBookmark(item.uri);
      else
        this.markPageAsTyped(item.uri);

      urls.push(item.uri);
    }

    var browserWindow = getTopWin();
    var where = browserWindow ?
                whereToOpenLink(aEvent, false, true) : "window";
    if (where == "window") {
      window.openDialog(getBrowserURL(), "_blank",
                        "chrome,all,dialog=no", urls.join("|"));
      return;
    }

    var loadInBackground = where == "tabshifted" ? true : false;
    var replaceCurrentTab = where == "tab" ? false : true;
    browserWindow.getBrowser().loadTabs(urls, loadInBackground,
                                        replaceCurrentTab);
  },

  openContainerNodeInTabs: function PU_openContainerInTabs(aNode, aEvent) {
    var urlsToOpen = this.getURLsForContainerNode(aNode);
    if (!this._confirmOpenInTabs(urlsToOpen.length))
      return;

    this._openTabset(urlsToOpen, aEvent);
  },

  openURINodesInTabs: function PU_openURINodesInTabs(aNodes, aEvent) {
    var urlsToOpen = [];
    for (var i=0; i < aNodes.length; i++) {
      
      if (this.nodeIsURI(aNodes[i]))
        urlsToOpen.push({uri: aNodes[i].uri, isBookmark: this.nodeIsBookmark(aNodes[i])});
    }
    this._openTabset(urlsToOpen, aEvent);
  },

  









  openNodeWithEvent: function PU_openNodeWithEvent(aNode, aEvent) {
    this.openNodeIn(aNode, whereToOpenLink(aEvent));
  },
  
  




  openNodeIn: function PU_openNodeIn(aNode, aWhere) {
    if (aNode && PlacesUtils.nodeIsURI(aNode) &&
        PlacesUtils.checkURLSecurity(aNode)) {
      var isBookmark = PlacesUtils.nodeIsBookmark(aNode);

      if (isBookmark)
        PlacesUtils.markPageAsFollowedBookmark(aNode.uri);
      else
        PlacesUtils.markPageAsTyped(aNode.uri);

      
      
      if (aWhere == "current" && isBookmark) {
        if (PlacesUtils.annotations
                       .itemHasAnnotation(aNode.itemId, LOAD_IN_SIDEBAR_ANNO)) {
          var w = getTopWin();
          if (w) {
            w.openWebPanel(aNode.title, aNode.uri);
            return;
          }
        }
      }
      openUILinkIn(aNode.uri, aWhere);
    }
  },

  


  createMenuItemForNode: function(aNode, aContainersMap) {
    var element;
    var type = aNode.type;
    if (type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR)
      element = document.createElement("menuseparator");
    else {
      var iconURI = aNode.icon;
      var iconURISpec = "";
      if (iconURI)
        iconURISpec = iconURI.spec;

      if (this.uriTypes.indexOf(type) != -1) {
        element = document.createElement("menuitem");
        element.className = "menuitem-iconic bookmark-item";
      }
      else if (this.containerTypes.indexOf(type) != -1) {
        element = document.createElement("menu");
        element.setAttribute("container", "true");

        if (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY)
          element.setAttribute("query", "true");
        else if (aNode.itemId != -1) {
          if (this.nodeIsLivemarkContainer(aNode))
            element.setAttribute("livemark", "true");
          else if (this.bookmarks
                     .getFolderIdForItem(aNode.itemId) == this.tagsFolderId)
            element.setAttribute("tagContainer", "true");
        }

        var popup = document.createElement("menupopup");
        popup.setAttribute("placespopup", "true");
        popup._resultNode = asContainer(aNode);
#ifndef XP_MACOSX
        
        popup.setAttribute("context", "placesContext");
#endif
        element.appendChild(popup);
        if (aContainersMap)
          aContainersMap.push({ resultNode: aNode, domNode: popup });
        element.className = "menu-iconic bookmark-item";
      }
      else
        throw "Unexpected node";

      element.setAttribute("label", this.getBestTitle(aNode));

      if (iconURISpec)
        element.setAttribute("image", iconURISpec);
    }
    element.node = aNode;
    element.node.viewIndex = 0;

    return element;
  },

  cleanPlacesPopup: function PU_cleanPlacesPopup(aPopup) {
    
    
    
    var items = [];
    aPopup._startMarker = -1;
    aPopup._endMarker = -1;
    for (var i = 0; i < aPopup.childNodes.length; ++i) {
      var item = aPopup.childNodes[i];
      if (item.getAttribute("builder") == "start") {
        aPopup._startMarker = i;
        continue;
      }
      if (item.getAttribute("builder") == "end") {
        aPopup._endMarker = i;
        continue;
      }
      if ((aPopup._startMarker != -1) && (aPopup._endMarker == -1))
        items.push(item);
    }

    
    
    for (var i = 0; i < items.length; ++i) {
      
      if (aPopup._emptyMenuItem != items[i]) {
        aPopup.removeChild(items[i]);
        if (aPopup._endMarker > 0)
          --aPopup._endMarker;
      }
    }

    
    
    if (aPopup._startMarker == -1) {
      var end = aPopup._endMarker == -1 ?
                aPopup.childNodes.length - 1 : aPopup._endMarker - 1;
      for (var i = end; i >= 0; i--) {
        
        if (aPopup._emptyMenuItem != aPopup.childNodes[i]) {
          aPopup.removeChild(aPopup.childNodes[i]);
          if (aPopup._endMarker > 0)
            --aPopup._endMarker;
        }
      }
    }
  },

  getBestTitle: function PU_getBestTitle(aNode) {
    var title;
    if (!aNode.title && this.uriTypes.indexOf(aNode.type) != -1) {
      
      
      try {
        var uri = this._uri(aNode.uri);
        var host = uri.host;
        var fileName = uri.QueryInterface(Ci.nsIURL).fileName;
        
        title = host + (fileName ?
                        (host ? "/" + this.ellipsis + "/" : "") + fileName :
                        uri.path);
      }
      catch (e) {
        
        title = "";
      }
    }
    else
      title = aNode.title;

    return title || PlacesUtils.getString("noTitle");
  },

  get leftPaneQueries() {    
    
    this.leftPaneFolderId;
    return this.leftPaneQueries;
  },

  
  get leftPaneFolderId() {
    var leftPaneRoot = -1;
    var allBookmarksId;
    var items = this.annotations.getItemsWithAnnotation(ORGANIZER_FOLDER_ANNO, {});
    if (items.length != 0 && items[0] != -1)
      leftPaneRoot = items[0];
    if (leftPaneRoot != -1) {
      
      delete this.leftPaneQueries;
      this.leftPaneQueries = {};
      var items = this.annotations.getItemsWithAnnotation(ORGANIZER_QUERY_ANNO, { });
      for (var i=0; i < items.length; i++) {
        var queryName = this.annotations
                            .getItemAnnotation(items[i], ORGANIZER_QUERY_ANNO);
        this.leftPaneQueries[queryName] = items[i];
      }
      delete this.leftPaneFolderId;
      return this.leftPaneFolderId = leftPaneRoot;
    }

    var self = this;
    const EXPIRE_NEVER = this.annotations.EXPIRE_NEVER;
    var callback = {
      runBatched: function(aUserData) {
        delete self.leftPaneQueries;
        self.leftPaneQueries = { };

        
        leftPaneRoot = self.bookmarks.createFolder(self.placesRootId, "", -1);

        
        let uri = self._uri("place:sort=4&");
        let title = self.getString("OrganizerQueryHistory");
        let itemId = self.bookmarks.insertBookmark(leftPaneRoot, uri, -1, title);
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "History", 0, EXPIRE_NEVER);
        self.leftPaneQueries["History"] = itemId;

        

        
        uri = self._uri("place:folder=" + self.tagsFolderId);
        itemId = self.bookmarks.insertBookmark(leftPaneRoot, uri, -1, null);
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "Tags", 0, EXPIRE_NEVER);
        self.leftPaneQueries["Tags"] = itemId;

        
        title = self.getString("OrganizerQueryAllBookmarks");
        itemId = self.bookmarks.createFolder(leftPaneRoot, title, -1);
        allBookmarksId = itemId;
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "AllBookmarks", 0, EXPIRE_NEVER);
        self.leftPaneQueries["AllBookmarks"] = itemId;

        
        uri = self._uri("place:folder=" + self.toolbarFolderId);
        itemId = self.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "BookmarksToolbar", 0, EXPIRE_NEVER);
        self.leftPaneQueries["BookmarksToolbar"] = itemId;

        
        uri = self._uri("place:folder=" + self.bookmarksMenuFolderId);
        itemId = self.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "BookmarksMenu", 0, EXPIRE_NEVER);
        self.leftPaneQueries["BookmarksMenu"] = itemId;

        
        uri = self._uri("place:folder=" + self.unfiledBookmarksFolderId);
        itemId = self.bookmarks.insertBookmark(allBookmarksId, uri, -1, null);
        self.annotations.setItemAnnotation(itemId, ORGANIZER_QUERY_ANNO,
                                           "UnfiledBookmarks", 0,
                                           EXPIRE_NEVER);
        self.leftPaneQueries["UnfiledBookmarks"] = itemId;

        
        self.bookmarks.setFolderReadonly(leftPaneRoot, true);
      }
    };
    this.bookmarks.runInBatchMode(callback, null);
    this.annotations.setItemAnnotation(leftPaneRoot, ORGANIZER_FOLDER_ANNO,
                                       true, 0, EXPIRE_NEVER);
    delete this.leftPaneFolderId;
    return this.leftPaneFolderId = leftPaneRoot;
  },

  get allBookmarksFolderId() {
    
    this.leftPaneFolderId;
    delete this.allBookmarksFolderId;
    return this.allBookmarksFolderId = this.leftPaneQueries["AllBookmarks"];
  }
};

PlacesUtils.placesFlavors = [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                             PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                             PlacesUtils.TYPE_X_MOZ_PLACE];

PlacesUtils.GENERIC_VIEW_DROP_TYPES = [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                                       PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                                       PlacesUtils.TYPE_X_MOZ_PLACE,
                                       PlacesUtils.TYPE_X_MOZ_URL,
                                       PlacesUtils.TYPE_UNICODE];
