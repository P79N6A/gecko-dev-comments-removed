







































function LOG(str) {
  dump("*** " + str + "\n");
}

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

Components.utils.import("resource://gre/modules/JSON.jsm");

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";
const POST_DATA_ANNO = "URIProperties/POSTData";
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
    var bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
    delete this.bookmarks;
    return this.bookmarks = bms;
  },

  


  get history() {
    var hst = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
    delete this.history;
    return this.history = hst;
  },

  


  get livemarks() {
    var lms = Cc["@mozilla.org/browser/livemark-service;2"].
              getService(Ci.nsILivemarkService);
    delete this.livemarks;
    return this.livemarks = lms;
  },

  


  get annotations() {
    var annos = Cc["@mozilla.org/browser/annotation-service;1"].
                getService(Ci.nsIAnnotationService);
    delete this.annotations;
    return this.annotations = annos;
  },

  


  get favicons() {
    var favsvc = Cc["@mozilla.org/browser/favicon-service;1"].
                 getService(Ci.nsIFaviconService);
    delete this.favicons;
    return this.favicons = favsvc;
  },

  


  get microsummaries() {
    var mss = Cc["@mozilla.org/microsummary/service;1"].
              getService(Ci.nsIMicrosummaryService);
    delete this.microsummaries;
    return this.microsummaries = mss;
  },

  


  get tagging() {
    var tagsvc = Cc["@mozilla.org/browser/tagging-service;1"].
                 getService(Ci.nsITaggingService);
    delete this.tagging;
    return this.tagging = tagsvc;
  },

  get RDF() {
    var RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
              getService(Ci.nsIRDFService);
    delete this.RDF;
    return this.RDF = RDF;
  },

  get localStore() {
    var localStore = this.RDF.GetDataSource("rdf:local-store");
    delete this.localStore;
    return this.localStore = localStore;
  },

  get tm() {
    delete this.tm;
    return this.tm = this.ptm.transactionManager;
  },

  get ptm() {
    var ptm = Cc["@mozilla.org/browser/placesTransactionsService;1"].
              getService(Ci.nsIPlacesTransactionsService);
    delete this.ptm
    return this.ptm = ptm;
  },

  get clipboard() {
    var clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                    getService(Ci.nsIClipboard);
    delete this.clipboard;
    return this.clipboard = clipboard;
  },

  





  _uri: function(aSpec) {
    NS_ASSERT(aSpec, "empty URL spec");
    return IO.newURI(aSpec);
  },

  





  _wrapString: function(aString) {
    var s = Cc["@mozilla.org/supports-string;1"].
            createInstance(Ci.nsISupportsString);
    s.data = aString;
    return s;
  },

  


  get _bundle() {
    const PLACES_STRING_BUNDLE_URI =
      "chrome://browser/locale/places/places.properties";
    var bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                 getService(Ci.nsIStringBundleService).
                 createBundle(PLACES_STRING_BUNDLE_URI);
    delete this._bundle;
    return this._bundle = bundle;
  },

  getFormattedString: function PU_getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },

  getString: function PU_getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  





  nodeIsFolder: function(aNode) {
    NS_ASSERT(aNode, "null node");
    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER);
  },

  





  nodeIsBookmark: function(aNode) {
    NS_ASSERT(aNode, "null node");
    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_URI &&
           aNode.itemId != -1;
  },

  





  nodeIsSeparator: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return (aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR);
  },

  





  nodeIsVisit: function(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_VISIT ||
           type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  





  nodeIsURI: function(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_URI ||
           type == NHRN.RESULT_TYPE_VISIT ||
           type == NHRN.RESULT_TYPE_FULL_VISIT;
  },

  





  nodeIsQuery: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY;
  },

  






  nodeIsReadOnly: function(aNode) {
    NS_ASSERT(aNode, "null node");

    if (this.nodeIsFolder(aNode))
      return this.bookmarks.getFolderReadonly(aNode.itemId);
    if (this.nodeIsQuery(aNode))
      return asQuery(aNode).childrenReadOnly;
    return false;
  },

  





  nodeIsHost: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return aNode.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_HOST;
  },

  





  nodeIsContainer: function(aNode) {
    NS_ASSERT(aNode, "null node");

    const NHRN = Ci.nsINavHistoryResultNode;
    var type = aNode.type;
    return type == NHRN.RESULT_TYPE_HOST ||
           type == NHRN.RESULT_TYPE_QUERY ||
           type == NHRN.RESULT_TYPE_FOLDER ||
           type == NHRN.RESULT_TYPE_DAY ||
           type == NHRN.RESULT_TYPE_DYNAMIC_CONTAINER;
  },

  








  nodeIsDynamicContainer: function(aNode) {
    NS_ASSERT(aNode, "null node");
    if (aNode.type == NHRN.RESULT_TYPE_DYNAMIC_CONTAINER)
      return true;
    return false;
  },

 






  nodeIsLivemarkContainer: function(aNode) {
    
    
    return this.nodeIsFolder(aNode) &&
           this.annotations.itemHasAnnotation(aNode.itemId, LMANNO_FEEDURI);
  },

 





  nodeIsLivemarkItem: function(aNode) {
    return aNode.parent && this.nodeIsLivemarkContainer(aNode.parent);
  },

  





  isReadonlyFolder: function(aNode) {
    NS_ASSERT(aNode, "null node");

    return this.nodeIsFolder(aNode) &&
           this.bookmarks.getFolderReadonly(aNode.itemId);
  },

  






  getIndexOfNode: function(aNode) {
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

  












  wrapNode: function(aNode, aType, aOverrideURI) {
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

  










  _getURIItemCopyTransaction: function(aData, aContainer, aIndex) {
    return this.ptm.createItem(IO.newURI(aData.uri), aContainer, aIndex,
                               aData.title, "");
  },

  













  _getBookmarkItemCopyTransaction: function(aData, aContainer, aIndex,
                                            aExcludeAnnotations) {
    var itemURL = IO.newURI(aData.uri);
    var itemTitle = aData.title;
    var keyword = aData.keyword;
    var annos = aData.annos;
    if (aExcludeAnnotations) {
      annos =
        annos.filter(function(aValue, aIndex, aArray) {
                       return aExcludeAnnotations.indexOf(aValue.name) == -1;
                    });
    }

    return this.ptm.createItem(itemURL, aContainer, aIndex, itemTitle, keyword,
                               annos);
  },

  











  _getFolderCopyTransaction: function(aData, aContainer, aIndex) {
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

    return this.ptm.createFolder(title, aContainer, aIndex, annos,
                                 getChildItemsTransactions(aData.children));
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
        
        
        if (parts.length % 2)
          break;
        for (var i = 0; i < parts.length; i=i+2) {
          var uriString = parts[i];
          var titleString = parts[i+1];
          
          if (IO.newURI(uriString)) {
            nodes.push({ uri: uriString,
                         title: titleString ? titleString : uriString });
          }
        }
        break;
      case this.TYPE_UNICODE:
        var parts = blob.split("\n");
        for (var i = 0; i < parts.length; i++) {
          var uriString = parts[i];
          
          if (uriString && IO.newURI(uriString))
            nodes.push({ uri: uriString, title: uriString });
        }
        break;
      default:
        LOG("Cannot unwrap data of type " + type);
        throw Cr.NS_ERROR_INVALID_ARG;
    }
    return nodes;
  },

  















  makeTransaction: function(data, type, container, index, copy) {
    switch (data.type) {
    case this.TYPE_X_MOZ_PLACE_CONTAINER:
      if (data.folder) {
        
        if (copy)
          return this._getFolderCopyTransaction(data, container, index);
      }
      else if (copy) {
        
        var feedURI = IO.newURI(data.uri.feed);
        var siteURI = IO.newURI(data.uri.site);
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
        return this.ptm.createItem(IO.newURI(data.uri), container, index,
                                   title);
      }
      return null;
    }
    if (data.id <= 0)
      return null;

    
    var id = data.folder ? data.folder.id : data.id;
    return this.ptm.moveItem(id, container, index);
  },

  













  getFolderContents: function(aFolderId, aExcludeItems, aExpandQueries) {
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    var options = this.history.getNewQueryOptions();
    options.excludeItems = aExcludeItems;
    options.expandQueries = aExpandQueries;

    var result = this.history.executeQuery(query, options);
    result.root.containerOpen = true;
    asContainer(result.root);
    return result;
  },

  








  
































  showAddBookmarkUI: function(aURI, aTitle, aDescription,
                              aDefaultInsertionPoint, aShowPicker,
                              aLoadInSidebar, aKeyword, aPostData) {
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

  










  showMinimalAddBookmarkUI: function(aURI, aTitle, aDescription,
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

  





















  showAddLivemarkUI: function(aFeedURI, aSiteURI, aTitle, aDescription,
                              aDefaultInsertionPoint, aShowPicker) {
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

  







  showMinimalAddLivemarkUI: function(aFeedURI, aSiteURI, aTitle, aDescription,
                                     aDefaultInsertionPoint, aShowPicker) {
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

  








  showMinimalAddMultiBookmarkUI: function(aURIList) {
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

  






  showBookmarkProperties: function(aId) {
    var info = {
      action: "edit",
      type: "bookmark",
      bookmarkId: aId
    };
    return this._showBookmarkDialog(info);
  },

  






  showFolderProperties: function(aId) {
    var info = {
      action: "edit",
      type: "folder",
      folderId: aId
    };
    return this._showBookmarkDialog(info);
  },

  













  showAddFolderUI: function(aTitle, aDefaultInsertionPoint, aShowPicker) {
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

  















  _showBookmarkDialog: function(aInfo, aMinimalUI) {
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

  





  getViewForNode: function(aNode) {
    var node = aNode;
    while (node) {
      
      if (node.getAttribute("type") == "places")
        return node;

      node = node.parentNode;
    }

    return null;
  },

  







  checkURLSecurity: function(aURINode) {
    if (!this.nodeIsBookmark(aURINode)) {
      var uri = IO.newURI(aURINode.uri);
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

  







  getAnnotationsForURI: function(aURI) {
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

  








  getAnnotationsForItem: function(aItemId) {
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

  








  setAnnotationsForURI: function(aURI, aAnnos) {
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

  








  setAnnotationsForItem: function(aItemId, aAnnos) {
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

  




  getQueryStringForFolder: function(aFolderId) {
    var options = this.history.getNewQueryOptions();
    var query = this.history.getNewQuery();
    query.setFolders([aFolderId], 1);
    return this.history.queriesToQueryString([query], 1, options);
  },

  







  getDescriptionFromDocument: function(doc) {
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
    var placesRootId = this.bookmarks.placesRoot;
    delete this.placesRootId;
    return this.placesRootId = placesRootId;
  },

  get bookmarksRootId() {
    var bookmarksRootId = this.bookmarks.bookmarksRoot;
    delete this.bookmarksRootId;
    return this.bookmarksRootId = bookmarksRootId;
  },

  get toolbarFolderId() {
    return this.bookmarks.toolbarFolder;
  },

  get tagRootId() {
    var tagRootId = this.bookmarks.tagRoot;
    delete this.tagRootId;
    return this.tagRootId = tagRootId;
  },

  get unfiledRootId() {
    var unfiledRootId = this.bookmarks.unfiledRoot;
    delete this.unfiledRootId;
    return this.unfiledRootId = unfiledRootId;
  },

  





  setPostDataForURI: function(aURI, aPostData) {
    const annos = this.annotations;
    if (aPostData)
      annos.setPageAnnotation(aURI, POST_DATA_ANNO, aPostData, 
                              0, Ci.nsIAnnotationService.EXPIRE_NEVER);
    else if (annos.pageHasAnnotation(aURI, POST_DATA_ANNO))
      annos.removePageAnnotation(aURI, POST_DATA_ANNO);
  },

  




  getPostDataForURI: function(aURI) {
    const annos = this.annotations;
    if (annos.pageHasAnnotation(aURI, POST_DATA_ANNO))
      return annos.getPageAnnotation(aURI, POST_DATA_ANNO);

    return null;
  },

  






  getItemDescription: function(aItemId) {
    if (this.annotations.itemHasAnnotation(aItemId, DESCRIPTION_ANNO))
      return this.annotations.getItemAnnotation(aItemId, DESCRIPTION_ANNO);
    return "";
  },

  



  getMostRecentBookmarkForURI: function(aURI) {
    var bmkIds = this.bookmarks.getBookmarkIdsForURI(aURI, {});
    for each (var bk in bmkIds) {
      
      var parent = this.bookmarks.getFolderIdForItem(bk);
      if (parent == this.unfiledRootId)
        return bk;

      var grandparent = this.bookmarks.getFolderIdForItem(parent);
      if (grandparent != this.tagRootId &&
          !this.annotations.itemHasAnnotation(parent, LMANNO_FEEDURI))
        return bk;
    }
    return -1;
  },

  getMostRecentFolderForFeedURI: function(aURI) {
    var feedSpec = aURI.spec
    var annosvc = this.annotations;
    var livemarks = annosvc.getItemsWithAnnotation(LMANNO_FEEDURI, {});
    for (var i = 0; i < livemarks.length; i++) {
      if (annosvc.getItemAnnotation(livemarks[i], LMANNO_FEEDURI) == feedSpec)
        return livemarks[i];
    }
    return -1;
  },

  getURLsForContainerNode: function(aNode) {
    let urls = [];
    if (this.nodeIsFolder(aNode) && asQuery(aNode).queryOptions.excludeItems) {
      
      let contents = this.getFolderContents(aNode.itemId, false, false).root;
      for (let i = 0; i < contents.childCount; ++i) {
        let child = contents.getChild(i);
        if (this.nodeIsURI(child))
          urls.push(child.uri);
      }
    }
    else {
      let wasOpen = aNode.containerOpen;
      if (!wasOpen)
        aNode.containerOpen = true;
      for (let i = 0; i < aNode.childCount; ++i) {
        let child = aNode.getChild(i);
        if (this.nodeIsURI(child))
          urls.push(child.uri);
      }
      aNode.containerOpen = wasOpen;
    }

    return urls;
  },

  


  _confirmOpenInTabs: function(numTabsToOpen) {
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

  _openTabset: function(aURLs, aEvent) {
    var browserWindow = getTopWin();
    var where = browserWindow ?
                whereToOpenLink(aEvent, false, true) : "window";
    if (where == "window") {
      window.openDialog(getBrowserURL(), "_blank",
                        "chrome,all,dialog=no", aURLs.join("|"));
      return;
    }

    var loadInBackground = where == "tabshifted" ? true : false;
    var replaceCurrentTab = where == "tab" ? false : true;
    browserWindow.getBrowser().loadTabs(aURLs, loadInBackground,
                                        replaceCurrentTab);
  },

  openContainerNodeInTabs: function(aNode, aEvent) {
    var urlsToOpen = this.getURLsForContainerNode(aNode);
    if (!this._confirmOpenInTabs(urlsToOpen.length))
      return;
    this._openTabset(urlsToOpen, aEvent);
  },

  openURINodesInTabs: function(aNodes, aEvent) {
    var urlsToOpen = [];
    for (var i=0; i < aNodes.length; i++) {
      if (this.nodeIsURI(aNodes[i]))
        urlsToOpen.push(aNodes[i].uri);
    }
    this._openTabset(urlsToOpen, aEvent);
  },

  get placesFlavors() {
    var placeTypes = [this.TYPE_X_MOZ_PLACE_CONTAINER,
                      this.TYPE_X_MOZ_PLACE_SEPARATOR,
                      this.TYPE_X_MOZ_PLACE];
    var placesFlavors = Cc["@mozilla.org/supports-array;1"].
                        createInstance(Ci.nsISupportsArray);
    for (var i = 0; i < placeTypes.length; ++i) {
      var cstring = Cc["@mozilla.org/supports-cstring;1"].
                    createInstance(Ci.nsISupportsCString);
      cstring.data = placeTypes[i];
      this._placesFlavors.AppendElement(cstring);
    }
    delete this.placesFlavors;
    return this.placesFlavors = placesFlavors;
  }
};

PlacesUtils.GENERIC_VIEW_DROP_TYPES = [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                                       PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                                       PlacesUtils.TYPE_X_MOZ_PLACE,
                                       PlacesUtils.TYPE_X_MOZ_URL,
                                       PlacesUtils.TYPE_UNICODE];
