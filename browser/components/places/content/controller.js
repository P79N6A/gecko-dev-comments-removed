




XPCOMUtils.defineLazyModuleGetter(this, "ForgetAboutSite",
                                  "resource://gre/modules/ForgetAboutSite.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");


const ORGANIZER_ROOT_BOOKMARKS = "place:folder=BOOKMARKS_MENU&excludeItems=1&queryType=1";


const RELOAD_ACTION_NOTHING = 0;

const RELOAD_ACTION_INSERT = 1;

const RELOAD_ACTION_REMOVE = 2;


const RELOAD_ACTION_MOVE = 3;



const REMOVE_PAGES_CHUNKLEN = 300;



















function InsertionPoint(aItemId, aIndex, aOrientation, aTagName = null,
                        aDropNearItemId = false) {
  this.itemId = aItemId;
  this._index = aIndex;
  this.orientation = aOrientation;
  this.tagName = aTagName;
  this.dropNearItemId = aDropNearItemId;
}

InsertionPoint.prototype = {
  set index(val) {
    return this._index = val;
  },

  promiseGuid: function () PlacesUtils.promiseItemGuid(this.itemId),

  get index() {
    if (this.dropNearItemId > 0) {
      
      
      var index = PlacesUtils.bookmarks.getItemIndex(this.dropNearItemId);
      return this.orientation == Ci.nsITreeView.DROP_BEFORE ? index : index + 1;
    }
    return this._index;
  },

  get isTag() typeof(this.tagName) == "string"
};





function PlacesController(aView) {
  this._view = aView;
  XPCOMUtils.defineLazyServiceGetter(this, "clipboard",
                                     "@mozilla.org/widget/clipboard;1",
                                     "nsIClipboard");
  XPCOMUtils.defineLazyGetter(this, "profileName", function () {
    return Services.dirsvc.get("ProfD", Ci.nsIFile).leafName;
  });

  this._cachedLivemarkInfoObjects = new Map();
}

PlacesController.prototype = {
  


  _view: null,

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIClipboardOwner
  ]),

  
  LosingOwnership: function PC_LosingOwnership (aXferable) {
    this.cutNodes = [];
  },

  terminate: function PC_terminate() {
    this._releaseClipboardOwnership();
  },

  supportsCommand: function PC_supportsCommand(aCommand) {
    
    switch (aCommand) {
    case "cmd_undo":
    case "cmd_redo":
    case "cmd_cut":
    case "cmd_copy":
    case "cmd_paste":
    case "cmd_delete":
    case "cmd_selectAll":
      return true;
    }

    
    
    const CMD_PREFIX = "placesCmd_";
    return (aCommand.substr(0, CMD_PREFIX.length) == CMD_PREFIX);
  },

  isCommandEnabled: function PC_isCommandEnabled(aCommand) {
    if (PlacesUIUtils.useAsyncTransactions) {
      switch (aCommand) {
      case "placesCmd_new:folder":
      case "placesCmd_new:bookmark":
      case "placesCmd_createBookmark":
        return false;
      }
    }

    switch (aCommand) {
    case "cmd_undo":
      if (!PlacesUIUtils.useAsyncTransactions)
        return PlacesUtils.transactionManager.numberOfUndoItems > 0;

      return PlacesTransactions.topUndoEntry != null;
    case "cmd_redo":
      if (!PlacesUIUtils.useAsyncTransactions)
        return PlacesUtils.transactionManager.numberOfRedoItems > 0;

      return PlacesTransactions.topRedoEntry != null;
    case "cmd_cut":
    case "placesCmd_cut":
    case "placesCmd_moveBookmarks":
      for (let node of this._view.selectedNodes) {
        
        
        if (node.itemId == -1 ||
            (node.parent && PlacesUtils.nodeIsTagQuery(node.parent))) {
          return false;
        }
      }
      
    case "cmd_delete":
    case "placesCmd_delete":
    case "placesCmd_deleteDataHost":
      return this._hasRemovableSelection();
    case "cmd_copy":
    case "placesCmd_copy":
      return this._view.hasSelection;
    case "cmd_paste":
    case "placesCmd_paste":
      return this._canInsert(true) && this._isClipboardDataPasteable();
    case "cmd_selectAll":
      if (this._view.selType != "single") {
        let rootNode = this._view.result.root;
        if (rootNode.containerOpen && rootNode.childCount > 0)
          return true;
      }
      return false;
    case "placesCmd_open":
    case "placesCmd_open:window":
    case "placesCmd_open:privatewindow":
    case "placesCmd_open:tab":
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.nodeIsURI(selectedNode);
    case "placesCmd_new:folder":
      return this._canInsert();
    case "placesCmd_new:bookmark":
      return this._canInsert();
    case "placesCmd_new:separator":
      return this._canInsert() &&
             !PlacesUtils.asQuery(this._view.result.root).queryOptions.excludeItems &&
             this._view.result.sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_show:info":
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.getConcreteItemId(selectedNode) != -1
    case "placesCmd_reload":
      
      var selectedNode = this._view.selectedNode;
      return selectedNode && this.hasCachedLivemarkInfo(selectedNode);
    case "placesCmd_sortBy:name":
      var selectedNode = this._view.selectedNode;
      return selectedNode &&
             PlacesUtils.nodeIsFolder(selectedNode) &&
             !PlacesUIUtils.isContentsReadOnly(selectedNode) &&
             this._view.result.sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_createBookmark":
      var node = this._view.selectedNode;
      return node && PlacesUtils.nodeIsURI(node) && node.itemId == -1;
    default:
      return false;
    }
  },

  doCommand: function PC_doCommand(aCommand) {
    switch (aCommand) {
    case "cmd_undo":
      if (!PlacesUIUtils.useAsyncTransactions) {
        PlacesUtils.transactionManager.undoTransaction();
        return;
      }
      PlacesTransactions.undo().then(null, Components.utils.reportError);
      break;
    case "cmd_redo":
      if (!PlacesUIUtils.useAsyncTransactions) {
        PlacesUtils.transactionManager.redoTransaction();
        return;
      }
      PlacesTransactions.redo().then(null, Components.utils.reportError);
      break;
    case "cmd_cut":
    case "placesCmd_cut":
      this.cut();
      break;
    case "cmd_copy":
    case "placesCmd_copy":
      this.copy();
      break;
    case "cmd_paste":
    case "placesCmd_paste":
      this.paste().then(null, Components.utils.reportError);
      break;
    case "cmd_delete":
    case "placesCmd_delete":
      this.remove("Remove Selection").then(null, Components.utils.reportError);
      break;
    case "placesCmd_deleteDataHost":
      var host;
      if (PlacesUtils.nodeIsHost(this._view.selectedNode)) {
        var queries = this._view.selectedNode.getQueries();
        host = queries[0].domain;
      }
      else
        host = NetUtil.newURI(this._view.selectedNode.uri).host;
      ForgetAboutSite.removeDataFromDomain(host);
      break;
    case "cmd_selectAll":
      this.selectAll();
      break;
    case "placesCmd_open":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "current", this._view);
      break;
    case "placesCmd_open:window":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "window", this._view);
      break;
    case "placesCmd_open:privatewindow":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "window", this._view, true);
      break;
    case "placesCmd_open:tab":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "tab", this._view);
      break;
    case "placesCmd_new:folder":
      this.newItem("folder");
      break;
    case "placesCmd_new:bookmark":
      this.newItem("bookmark");
      break;
    case "placesCmd_new:separator":
      this.newSeparator().then(null, Components.utils.reportError);
      break;
    case "placesCmd_show:info":
      this.showBookmarkPropertiesForSelection();
      break;
    case "placesCmd_moveBookmarks":
      this.moveSelectedBookmarks();
      break;
    case "placesCmd_reload":
      this.reloadSelectedLivemark();
      break;
    case "placesCmd_sortBy:name":
      this.sortFolderByName().then(null, Components.utils.reportError);
      break;
    case "placesCmd_createBookmark":
      let node = this._view.selectedNode;
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "bookmark"
                                       , hiddenRows: [ "description"
                                                     , "keyword"
                                                     , "location"
                                                     , "loadInSidebar" ]
                                       , uri: NetUtil.newURI(node.uri)
                                       , title: node.title
                                       }, window.top);
      break;
    }
  },

  onEvent: function PC_onEvent(eventName) { },


  









  _hasRemovableSelection() {
    var ranges = this._view.removableSelectionRanges;
    if (!ranges.length)
      return false;

    var root = this._view.result.root;

    for (var j = 0; j < ranges.length; j++) {
      var nodes = ranges[j];
      for (var i = 0; i < nodes.length; ++i) {
        
        if (nodes[i] == root)
          return false;

        if (!PlacesUIUtils.canUserRemove(nodes[i]))
          return false;
      }
    }

    return true;
  },

  


  _canInsert: function PC__canInsert(isPaste) {
    var ip = this._view.insertionPoint;
    return ip != null && (isPaste || ip.isTag != true);
  },

  







  _isClipboardDataPasteable: function PC__isClipboardDataPasteable() {
    
    

    var flavors = PlacesUIUtils.PLACES_FLAVORS;
    var clipboard = this.clipboard;
    var hasPlacesData =
      clipboard.hasDataMatchingFlavors(flavors, flavors.length,
                                       Ci.nsIClipboard.kGlobalClipboard);
    if (hasPlacesData)
      return this._view.insertionPoint != null;

    
    
    var xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    xferable.init(null);

    xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_URL);
    xferable.addDataFlavor(PlacesUtils.TYPE_UNICODE);
    clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);

    try {
      
      var data = { }, type = { };
      xferable.getAnyTransferData(type, data, { });
      data = data.value.QueryInterface(Ci.nsISupportsString).data;
      if (type.value != PlacesUtils.TYPE_X_MOZ_URL &&
          type.value != PlacesUtils.TYPE_UNICODE)
        return false;

      
      var unwrappedNodes = PlacesUtils.unwrapNodes(data, type.value);
      return this._view.insertionPoint != null;
    }
    catch (e) {
      
      return false;
    }
  },

  


















  _buildSelectionMetadata: function PC__buildSelectionMetadata() {
    var metadata = [];
    var nodes = this._view.selectedNodes;

    for (var i = 0; i < nodes.length; i++) {
      var nodeData = {};
      var node = nodes[i];
      var nodeType = node.type;
      var uri = null;

      
      
      switch (nodeType) {
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY:
          nodeData["query"] = true;
          if (node.parent) {
            switch (PlacesUtils.asQuery(node.parent).queryOptions.resultType) {
              case Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY:
                nodeData["host"] = true;
                break;
              case Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY:
              case Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_QUERY:
                nodeData["day"] = true;
                break;
            }
          }
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER:
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT:
          nodeData["folder"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR:
          nodeData["separator"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_URI:
          nodeData["link"] = true;
          uri = NetUtil.newURI(node.uri);
          if (PlacesUtils.nodeIsBookmark(node)) {
            nodeData["bookmark"] = true;
            PlacesUtils.nodeIsTagQuery(node.parent)

            var parentNode = node.parent;
            if (parentNode) {
              if (PlacesUtils.nodeIsTagQuery(parentNode))
                nodeData["tagChild"] = true;
              else if (this.hasCachedLivemarkInfo(parentNode))
                nodeData["livemarkChild"] = true;
            }
          }
          break;
      }

      
      if (uri) {
        let names = PlacesUtils.annotations.getPageAnnotationNames(uri);
        for (let j = 0; j < names.length; ++j)
          nodeData[names[j]] = true;
      }

      
      if (node.itemId != -1) {
        let names = PlacesUtils.annotations
                               .getItemAnnotationNames(node.itemId);
        for (let j = 0; j < names.length; ++j)
          nodeData[names[j]] = true;
      }
      metadata.push(nodeData);
    }

    return metadata;
  },

  








  _shouldShowMenuItem: function PC__shouldShowMenuItem(aMenuItem, aMetaData) {
    var selectiontype = aMenuItem.getAttribute("selectiontype");
    if (!selectiontype) {
      selectiontype = "single|multiple";
    }
    var selectionTypes = selectiontype.split("|");
    if (selectionTypes.indexOf("any") != -1) {
      return true;
    }
    var count = aMetaData.length;
    if (count > 1 && selectionTypes.indexOf("multiple") == -1)
      return false;
    if (count == 1 && selectionTypes.indexOf("single") == -1)
      return false;
    
    
    
    if (count == 0)
      return selectionTypes.indexOf("none") != -1;

    var forceHideAttr = aMenuItem.getAttribute("forcehideselection");
    if (forceHideAttr) {
      var forceHideRules = forceHideAttr.split("|");
      for (let i = 0; i < aMetaData.length; ++i) {
        for (let j = 0; j < forceHideRules.length; ++j) {
          if (forceHideRules[j] in aMetaData[i])
            return false;
        }
      }
    }

    var selectionAttr = aMenuItem.getAttribute("selection");
    if (!selectionAttr) {
      return !aMenuItem.hidden;
    }

    if (selectionAttr == "any")
      return true;

    var showRules = selectionAttr.split("|");
    var anyMatched = false;
    function metaDataNodeMatches(metaDataNode, rules) {
      for (var i = 0; i < rules.length; i++) {
        if (rules[i] in metaDataNode)
          return true;
      }
      return false;
    }

    for (var i = 0; i < aMetaData.length; ++i) {
      if (metaDataNodeMatches(aMetaData[i], showRules))
        anyMatched = true;
      else
        return false;
    }
    return anyMatched;
  },

  


































  buildContextMenu: function PC_buildContextMenu(aPopup) {
    var metadata = this._buildSelectionMetadata();
    var ip = this._view.insertionPoint;
    var noIp = !ip || ip.isTag;

    var separator = null;
    var visibleItemsBeforeSep = false;
    var usableItemCount = 0;
    for (var i = 0; i < aPopup.childNodes.length; ++i) {
      var item = aPopup.childNodes[i];
      if (item.localName != "menuseparator") {
        
        var hideIfNoIP = item.getAttribute("hideifnoinsertionpoint") == "true" &&
                         noIp && !(ip && ip.isTag && item.id == "placesContext_paste");
        var hideIfPrivate = item.getAttribute("hideifprivatebrowsing") == "true" &&
                            PrivateBrowsingUtils.isWindowPrivate(window);
        var shouldHideItem = hideIfNoIP || hideIfPrivate ||
                             !this._shouldShowMenuItem(item, metadata);
        item.hidden = item.disabled = shouldHideItem;

        if (!item.hidden) {
          visibleItemsBeforeSep = true;
          usableItemCount++;

          
          if (separator) {
            separator.hidden = false;
            separator = null;
          }
        }
      }
      else { 
        
        
        item.hidden = true;

        
        if (visibleItemsBeforeSep)
          separator = item;

        
        visibleItemsBeforeSep = false;
      }
    }

    
    if (usableItemCount > 0) {
      var openContainerInTabsItem = document.getElementById("placesContext_openContainer:tabs");
      if (!openContainerInTabsItem.hidden) {
        var containerToUse = this._view.selectedNode || this._view.result.root;
        if (PlacesUtils.nodeIsContainer(containerToUse)) {
          if (!PlacesUtils.hasChildURIs(containerToUse)) {
            openContainerInTabsItem.disabled = true;
            
            usableItemCount--;
          }
        }
      }
    }

    return usableItemCount > 0;
  },

  


  selectAll: function PC_selectAll() {
    this._view.selectAll();
  },

  


  showBookmarkPropertiesForSelection:
  function PC_showBookmarkPropertiesForSelection() {
    var node = this._view.selectedNode;
    if (!node)
      return;

    var itemType = PlacesUtils.nodeIsFolder(node) ||
                   PlacesUtils.nodeIsTagQuery(node) ? "folder" : "bookmark";
    var concreteId = PlacesUtils.getConcreteItemId(node);
    var isRootItem = PlacesUtils.isRootItem(concreteId);
    var itemId = node.itemId;
    if (isRootItem || PlacesUtils.nodeIsTagQuery(node)) {
      
      
      itemId = concreteId;
    }

    PlacesUIUtils.showBookmarkDialog({ action: "edit"
                                     , type: itemType
                                     , itemId: itemId
                                     , readOnly: isRootItem
                                     , hiddenRows: [ "folderPicker" ]
                                     }, window.top);
  },

  



  _assertURINotString: function PC__assertURINotString(value) {
    NS_ASSERT((typeof(value) == "object") && !(value instanceof String),
           "This method should be passed a URI as a nsIURI object, not as a string.");
  },

  


  reloadSelectedLivemark: function PC_reloadSelectedLivemark() {
    var selectedNode = this._view.selectedNode;
    if (selectedNode) {
      let itemId = selectedNode.itemId;
      PlacesUtils.livemarks.getLivemark({ id: itemId })
        .then(aLivemark => {
          aLivemark.reload(true);
        }, Components.utils.reportError);
    }
  },

  


  openSelectionInTabs: function PC_openLinksInTabs(aEvent) {
    var node = this._view.selectedNode;
    var nodes = this._view.selectedNodes;
    
    if (!node && !nodes.length) {
      node = this._view.result.root;
    }
    if (node && PlacesUtils.nodeIsContainer(node))
      PlacesUIUtils.openContainerNodeInTabs(node, aEvent, this._view);
    else
      PlacesUIUtils.openURINodesInTabs(nodes, aEvent, this._view);
  },

  





  newItem: function PC_newItem(aType) {
    let ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    let performed =
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: aType
                                       , defaultInsertionPoint: ip
                                       , hiddenRows: [ "folderPicker" ]
                                       }, window.top);
    if (performed) {
      
      let insertedNodeId = PlacesUtils.bookmarks
                                      .getIdForItemAt(ip.itemId, ip.index);
      this._view.selectItems([insertedNodeId], false);
    }
  },

  


  newSeparator: Task.async(function* () {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    if (!PlacesUIUtils.useAsyncTransactions) {
      let txn = new PlacesCreateSeparatorTransaction(ip.itemId, ip.index);
      PlacesUtils.transactionManager.doTransaction(txn);
      
      let insertedNodeId = PlacesUtils.bookmarks
                                      .getIdForItemAt(ip.itemId, ip.index);
      this._view.selectItems([insertedNodeId], false);
      return;
    }

    let txn = PlacesTransactions.NewSeparator({ parentGuid: yield ip.promiseGuid()
                                              , index: ip.index });
    let guid = yield txn.transact();
    let itemId = yield PlacesUtils.promiseItemId(guid);
    
    this._view.selectItems([itemId], false);
  }),

  


  moveSelectedBookmarks: function PC_moveBookmarks() {
    window.openDialog("chrome://browser/content/places/moveBookmarks.xul",
                      "", "chrome, modal",
                      this._view.selectedNodes);
  },

  


  sortFolderByName: Task.async(function* () {
    let itemId = PlacesUtils.getConcreteItemId(this._view.selectedNode);
    if (!PlacesUIUtils.useAsyncTransactions) {
      var txn = new PlacesSortFolderByNameTransaction(itemId);
      PlacesUtils.transactionManager.doTransaction(txn);
      return;
    }
    let guid = yield PlacesUtils.promiseItemGuid(itemId);
    yield PlacesTransactions.SortByName(guid).transact();
  }),

  









  _shouldSkipNode: function PC_shouldSkipNode(node, pastFolders) {
    







    function isContainedBy(node, parent) {
      var cursor = node.parent;
      while (cursor) {
        if (cursor == parent)
          return true;
        cursor = cursor.parent;
      }
      return false;
    }

      for (var j = 0; j < pastFolders.length; ++j) {
        if (isContainedBy(node, pastFolders[j]))
          return true;
      }
      return false;
  },

  









  _removeRange: function PC__removeRange(range, transactions, removedFolders) {
    NS_ASSERT(transactions instanceof Array, "Must pass a transactions array");
    if (!removedFolders)
      removedFolders = [];

    for (var i = 0; i < range.length; ++i) {
      var node = range[i];
      if (this._shouldSkipNode(node, removedFolders))
        continue;

      if (PlacesUtils.nodeIsTagQuery(node.parent)) {
        
        
        var tagItemId = PlacesUtils.getConcreteItemId(node.parent);
        var uri = NetUtil.newURI(node.uri);
        if (PlacesUIUtils.useAsyncTransactions) {
          let tag = node.parent.title;
          if (!tag)
            tag = PlacesUtils.bookmarks.getItemTitle(tagItemId);
          transactions.push(PlacesTransactions.Untag({ uri: uri, tag: tag }));
        }
        else {
          let txn = new PlacesUntagURITransaction(uri, [tagItemId]);
          transactions.push(txn);
        }
      }
      else if (PlacesUtils.nodeIsTagQuery(node) && node.parent &&
               PlacesUtils.nodeIsQuery(node.parent) &&
               PlacesUtils.asQuery(node.parent).queryOptions.resultType ==
                 Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY) {
        
        
        
        
        let tag = node.title;
        let URIs = PlacesUtils.tagging.getURIsForTag(tag);
        if (PlacesUIUtils.useAsyncTransactions) {
          transactions.push(PlacesTransactions.Untag({ tag: tag, uris: URIs }));
        }
        else {
          for (var j = 0; j < URIs.length; j++) {
            let txn = new PlacesUntagURITransaction(URIs[j], [tag]);
            transactions.push(txn);
          }
        }
      }
      else if (PlacesUtils.nodeIsURI(node) &&
               PlacesUtils.nodeIsQuery(node.parent) &&
               PlacesUtils.asQuery(node.parent).queryOptions.queryType ==
                 Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        
        PlacesUtils.bhistory.removePage(NetUtil.newURI(node.uri));
        
      }
      else if (node.itemId == -1 &&
               PlacesUtils.nodeIsQuery(node) &&
               PlacesUtils.asQuery(node).queryOptions.queryType ==
                 Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        
        
        
        this._removeHistoryContainer(node);
        
      }
      else {
        
        if (PlacesUtils.nodeIsFolder(node)) {
          
          
          removedFolders.push(node);
        }
        if (PlacesUIUtils.useAsyncTransactions) {
          transactions.push(
            PlacesTransactions.Remove({ guid: node.bookmarkGuid }));
        }
        else {
          let txn = new PlacesRemoveItemTransaction(node.itemId);
          transactions.push(txn);
        }
      }
    }
  },

  




  _removeRowsFromBookmarks: Task.async(function* (txnName) {
    var ranges = this._view.removableSelectionRanges;
    var transactions = [];
    var removedFolders = [];

    for (var i = 0; i < ranges.length; i++)
      this._removeRange(ranges[i], transactions, removedFolders);

    if (transactions.length > 0) {
      if (PlacesUIUtils.useAsyncTransactions) {
        yield PlacesTransactions.batch(transactions);
      }
      else {
        var txn = new PlacesAggregatedTransaction(txnName, transactions);
        PlacesUtils.transactionManager.doTransaction(txn);
      }
    }
  }),

  




  _removeRowsFromHistory: function PC__removeRowsFromHistory() {
    let nodes = this._view.selectedNodes;
    let URIs = [];
    for (let i = 0; i < nodes.length; ++i) {
      let node = nodes[i];
      if (PlacesUtils.nodeIsURI(node)) {
        let uri = NetUtil.newURI(node.uri);
        
        if (URIs.indexOf(uri) < 0) {
          URIs.push(uri);
        }
      }
      else if (PlacesUtils.nodeIsQuery(node) &&
               PlacesUtils.asQuery(node).queryOptions.queryType ==
                 Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        this._removeHistoryContainer(node);
      }
    }

    
    function pagesChunkGenerator(aURIs) {
      while (aURIs.length) {
        let URIslice = aURIs.splice(0, REMOVE_PAGES_CHUNKLEN);
        PlacesUtils.bhistory.removePages(URIslice, URIslice.length);
        Services.tm.mainThread.dispatch(function() {
          try {
            gen.next();
          } catch (ex if ex instanceof StopIteration) {}
        }, Ci.nsIThread.DISPATCH_NORMAL);
        yield undefined;
      }
    }
    let gen = pagesChunkGenerator(URIs);
    gen.next();
  },

  






  _removeHistoryContainer: function PC__removeHistoryContainer(aContainerNode) {
    if (PlacesUtils.nodeIsHost(aContainerNode)) {
      
      PlacesUtils.bhistory.removePagesFromHost(aContainerNode.title, true);
    }
    else if (PlacesUtils.nodeIsDay(aContainerNode)) {
      
      let query = aContainerNode.getQueries()[0];
      let beginTime = query.beginTime;
      let endTime = query.endTime;
      NS_ASSERT(query && beginTime && endTime,
                "A valid date container query should exist!");
      
      
      
      
      PlacesUtils.bhistory.removePagesByTimeframe(beginTime + 1, endTime);
    }
  },

  





  remove: Task.async(function* (aTxnName) {
    if (!this._hasRemovableSelection())
      return;

    NS_ASSERT(aTxnName !== undefined, "Must supply Transaction Name");

    var root = this._view.result.root;

    if (PlacesUtils.nodeIsFolder(root)) {
      if (PlacesUIUtils.useAsyncTransactions)
        yield this._removeRowsFromBookmarks(aTxnName);
      else
        this._removeRowsFromBookmarks(aTxnName);
    }
    else if (PlacesUtils.nodeIsQuery(root)) {
      var queryType = PlacesUtils.asQuery(root).queryOptions.queryType;
      if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS) {
        if (PlacesUIUtils.useAsyncTransactions)
          yield this._removeRowsFromBookmarks(aTxnName);
        else
          this._removeRowsFromBookmarks(aTxnName);
      }
      else if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        this._removeRowsFromHistory();
      }
      else {
        NS_ASSERT(false, "implement support for QUERY_TYPE_UNIFIED");
      }
    }
    else
      NS_ASSERT(false, "unexpected root");
  }),

  





  setDataTransfer: function PC_setDataTransfer(aEvent) {
    let dt = aEvent.dataTransfer;

    let result = this._view.result;
    let didSuppressNotifications = result.suppressNotifications;
    if (!didSuppressNotifications)
      result.suppressNotifications = true;

    function addData(type, index, overrideURI) {
      let wrapNode = PlacesUtils.wrapNode(node, type, overrideURI);
      dt.mozSetDataAt(type, wrapNode, index);
    }

    function addURIData(index, overrideURI) {
      addData(PlacesUtils.TYPE_X_MOZ_URL, index, overrideURI);
      addData(PlacesUtils.TYPE_UNICODE, index, overrideURI);
      addData(PlacesUtils.TYPE_HTML, index, overrideURI);
    }

    try {
      let nodes = this._view.draggableSelection;
      for (let i = 0; i < nodes.length; ++i) {
        var node = nodes[i];

        
        
        addData(PlacesUtils.TYPE_X_MOZ_PLACE, i);

        
        let livemarkInfo = this.getCachedLivemarkInfo(node);
        if (livemarkInfo) {
          addURIData(i, livemarkInfo.feedURI.spec);
        }
        else if (node.uri) {
          addURIData(i);
        }
      }
    }
    finally {
      if (!didSuppressNotifications)
        result.suppressNotifications = false;
    }
  },

  get clipboardAction () {
    let action = {};
    let actionOwner;
    try {
      let xferable = Cc["@mozilla.org/widget/transferable;1"].
                     createInstance(Ci.nsITransferable);
      xferable.init(null);
      xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_PLACE_ACTION)
      this.clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);
      xferable.getTransferData(PlacesUtils.TYPE_X_MOZ_PLACE_ACTION, action, {});
      [action, actionOwner] =
        action.value.QueryInterface(Ci.nsISupportsString).data.split(",");
    } catch(ex) {
      
      
      return "copy";
    }
    
    
    
    if (action == "cut" && actionOwner != this.profileName)
      action = "copy";

    return action;
  },

  _releaseClipboardOwnership: function PC__releaseClipboardOwnership() {
    if (this.cutNodes.length > 0) {
      
      this.clipboard.emptyClipboard(Ci.nsIClipboard.kGlobalClipboard);
    }
  },

  _clearClipboard: function PC__clearClipboard() {
    let xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    xferable.init(null);
    
    const TYPE = "text/x-moz-place-empty";
    xferable.addDataFlavor(TYPE);
    xferable.setTransferData(TYPE, PlacesUtils.toISupportsString(""), 0);
    this.clipboard.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
  },

  _populateClipboard: function PC__populateClipboard(aNodes, aAction) {
    
    
    let contents = [
      { type: PlacesUtils.TYPE_X_MOZ_PLACE, entries: [] },
      { type: PlacesUtils.TYPE_X_MOZ_URL, entries: [] },
      { type: PlacesUtils.TYPE_HTML, entries: [] },
      { type: PlacesUtils.TYPE_UNICODE, entries: [] },
    ];

    
    
    let copiedFolders = [];
    aNodes.forEach(function (node) {
      if (this._shouldSkipNode(node, copiedFolders))
        return;
      if (PlacesUtils.nodeIsFolder(node))
        copiedFolders.push(node);

      let livemarkInfo = this.getCachedLivemarkInfo(node);
      let overrideURI = livemarkInfo ? livemarkInfo.feedURI.spec : null;

      contents.forEach(function (content) {
        content.entries.push(
          PlacesUtils.wrapNode(node, content.type, overrideURI)
        );
      });
    }, this);

    function addData(type, data) {
      xferable.addDataFlavor(type);
      xferable.setTransferData(type, PlacesUtils.toISupportsString(data),
                               data.length * 2);
    }

    let xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    xferable.init(null);
    let hasData = false;
    
    
    contents.forEach(function (content) {
      if (content.entries.length > 0) {
        hasData = true;
        let glue =
          content.type == PlacesUtils.TYPE_X_MOZ_PLACE ? "," : PlacesUtils.endl;
        addData(content.type, content.entries.join(glue));
      }
    });

    
    
    
    
    addData(PlacesUtils.TYPE_X_MOZ_PLACE_ACTION, aAction + "," + this.profileName);

    if (hasData) {
      this.clipboard.setData(xferable,
                             this.cutNodes.length > 0 ? this : null,
                             Ci.nsIClipboard.kGlobalClipboard);
    }
  },

  _cutNodes: [],
  get cutNodes() this._cutNodes,
  set cutNodes(aNodes) {
    let self = this;
    function updateCutNodes(aValue) {
      self._cutNodes.forEach(function (aNode) {
        self._view.toggleCutNode(aNode, aValue);
      });
    }

    updateCutNodes(false);
    this._cutNodes = aNodes;
    updateCutNodes(true);
    return aNodes;
  },

  


  copy: function PC_copy() {
    let result = this._view.result;
    let didSuppressNotifications = result.suppressNotifications;
    if (!didSuppressNotifications)
      result.suppressNotifications = true;
    try {
      this._populateClipboard(this._view.selectedNodes, "copy");
    }
    finally {
      if (!didSuppressNotifications)
        result.suppressNotifications = false;
    }
  },

  


  cut: function PC_cut() {
    let result = this._view.result;
    let didSuppressNotifications = result.suppressNotifications;
    if (!didSuppressNotifications)
      result.suppressNotifications = true;
    try {
      this._populateClipboard(this._view.selectedNodes, "cut");
      this.cutNodes = this._view.selectedNodes;
    }
    finally {
      if (!didSuppressNotifications)
        result.suppressNotifications = false;
    }
  },

  


  paste: Task.async(function* () {
    
    let ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    let action = this.clipboardAction;

    let xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    xferable.init(null);
    
    
    [ PlacesUtils.TYPE_X_MOZ_PLACE,
      PlacesUtils.TYPE_X_MOZ_URL,
      PlacesUtils.TYPE_UNICODE,
    ].forEach(function (type) xferable.addDataFlavor(type));

    this.clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);

    
    let data = {}, type = {}, items = [];
    try {
      xferable.getAnyTransferData(type, data, {});
      data = data.value.QueryInterface(Ci.nsISupportsString).data;
      type = type.value;
      items = PlacesUtils.unwrapNodes(data, type);
    } catch(ex) {
      
      return;
    }

    let itemsToSelect = [];
    if (PlacesUIUtils.useAsyncTransactions) {
      if (ip.isTag) {
        let uris = [for (item of items) if ("uri" in item)
                    NetUtil.newURI(item.uri)];
        yield PlacesTransactions.Tag({ uris: uris, tag: ip.tagName }).transact();
      }
      else {
        yield PlacesTransactions.batch(function* () {
          let insertionIndex = ip.index;
          let parent = yield ip.promiseGuid();

          for (let item of items) {
            let doCopy = action == "copy";

            
            
            if (!doCopy &&
                !PlacesControllerDragHelper.canMoveUnwrappedNode(item)) {
              Cu.reportError("Tried to move an unmovable Places node, " +
                             "reverting to a copy operation.");
              doCopy = true;
            }
            let guid = yield PlacesUIUtils.getTransactionForData(
              item, type, parent, insertionIndex, doCopy).transact();
            itemsToSelect.push(yield PlacesUtils.promiseItemId(guid));

            
            
            if (insertionIndex != PlacesUtils.bookmarks.DEFAULT_INDEX)
              insertionIndex++;
          }
        });
      }
    }
    else {
      let transactions = [];
      let insertionIndex = ip.index;
      for (let i = 0; i < items.length; ++i) {
        if (ip.isTag) {
          
          
          let tagTxn = new PlacesTagURITransaction(NetUtil.newURI(items[i].uri),
                                                   [ip.itemId]);
          transactions.push(tagTxn);
          continue;
        }

        
        
        if (ip.index != PlacesUtils.bookmarks.DEFAULT_INDEX)
          insertionIndex = ip.index + i;

        
        
        if (action != "copy" && !PlacesControllerDragHelper.canMoveUnwrappedNode(items[i])) {
          Components.utils.reportError("Tried to move an unmovable Places node, " +
                                       "reverting to a copy operation.");
          action = "copy";
        }
        transactions.push(
          PlacesUIUtils.makeTransaction(items[i], type, ip.itemId,
                                        insertionIndex, action == "copy")
        );
      }

      let aggregatedTxn = new PlacesAggregatedTransaction("Paste", transactions);
      PlacesUtils.transactionManager.doTransaction(aggregatedTxn);

      for (let i = 0; i < transactions.length; ++i) {
        itemsToSelect.push(
          PlacesUtils.bookmarks.getIdForItemAt(ip.itemId, ip.index + i)
        );
      }
    }

    
    if (action == "cut") {
      this._clearClipboard();
    }

    if (itemsToSelect.length > 0)
      this._view.selectItems(itemsToSelect, false);
  }),

  







  cacheLivemarkInfo: function PC_cacheLivemarkInfo(aNode, aLivemarkInfo) {
    this._cachedLivemarkInfoObjects.set(aNode, aLivemarkInfo);
  },

  






  hasCachedLivemarkInfo: function PC_hasCachedLivemarkInfo(aNode)
    this._cachedLivemarkInfoObjects.has(aNode),

  






  getCachedLivemarkInfo: function PC_getCachedLivemarkInfo(aNode)
    this._cachedLivemarkInfoObjects.get(aNode, null)
};







let PlacesControllerDragHelper = {
  


  currentDropTarget: null,

  








  draggingOverChildNode: function PCDH_draggingOverChildNode(node) {
    let currentNode = this.currentDropTarget;
    while (currentNode) {
      if (currentNode == node)
        return true;
      currentNode = currentNode.parentNode;
    }
    return false;
  },

  


  getSession: function PCDH__getSession() {
    return this.dragService.getCurrentSession();
  },

  




  getFirstValidFlavor: function PCDH_getFirstValidFlavor(aFlavors) {
    for (let i = 0; i < aFlavors.length; i++) {
      if (PlacesUIUtils.SUPPORTED_FLAVORS.indexOf(aFlavors[i]) != -1)
        return aFlavors[i];
    }

    
    
    
    if (aFlavors.contains("text/plain")) {
        return PlacesUtils.TYPE_UNICODE;
    }

    return null;
  },

  





  canDrop: function PCDH_canDrop(ip, dt) {
    let dropCount = dt.mozItemCount;

    
    for (let i = 0; i < dropCount; i++) {
      let flavor = this.getFirstValidFlavor(dt.mozTypesAt(i));
      if (!flavor)
        return false;

      
      
      
      
      
      
      
      
      
      
      if (flavor == TAB_DROP_TYPE)
        continue;

      let data = dt.mozGetDataAt(flavor, i);
      let dragged;
      try {
        dragged = PlacesUtils.unwrapNodes(data, flavor)[0];
      }
      catch (e) {
        return false;
      }

      
      if (ip.isTag && ip.orientation == Ci.nsITreeView.DROP_ON &&
          dragged.type != PlacesUtils.TYPE_X_MOZ_URL &&
          (dragged.type != PlacesUtils.TYPE_X_MOZ_PLACE ||
           (dragged.uri && dragged.uri.startsWith("place:")) ))
        return false;

      
      
      if (dragged.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER ||
          (dragged.uri && dragged.uri.startsWith("place:")) ) {
        let parentId = ip.itemId;
        while (parentId != PlacesUtils.placesRootId) {
          if (dragged.concreteId == parentId || dragged.id == parentId)
            return false;
          parentId = PlacesUtils.bookmarks.getFolderIdForItem(parentId);
        }
      }
    }
    return true;
  },

  






  canMoveUnwrappedNode: function (aUnwrappedNode) {
    return aUnwrappedNode.id > 0 &&
           !PlacesUtils.isRootItem(aUnwrappedNode.id) &&
           (!aUnwrappedNode.parent || !PlacesUIUtils.isContentsReadOnly(aUnwrappedNode.parent)) &&
           aUnwrappedNode.parent != PlacesUtils.tagsFolderId &&
           aUnwrappedNode.grandParentId != PlacesUtils.tagsFolderId;
  },

  






  canMoveNode:
  function PCDH_canMoveNode(aNode) {
    
    if (aNode.itemId == -1)
      return false;

    
    
    let parentNode = aNode.parent;
    return parentNode != null &&
           !(PlacesUtils.nodeIsFolder(parentNode) &&
             PlacesUIUtils.isContentsReadOnly(parentNode)) &&
           !PlacesUtils.nodeIsTagQuery(parentNode);
  },

  




  onDrop: Task.async(function* (insertionPoint, dt) {
    let doCopy = ["copy", "link"].indexOf(dt.dropEffect) != -1;

    let transactions = [];
    let dropCount = dt.mozItemCount;
    let movedCount = 0;
    let parentGuid = PlacesUIUtils.useAsyncTransactions ?
                       (yield insertionPoint.promiseGuid()) : null;
    let tagName = insertionPoint.tagName;
    for (let i = 0; i < dropCount; ++i) {
      let flavor = this.getFirstValidFlavor(dt.mozTypesAt(i));
      if (!flavor)
        return;

      let data = dt.mozGetDataAt(flavor, i);
      let unwrapped;
      if (flavor != TAB_DROP_TYPE) {
        
        unwrapped = PlacesUtils.unwrapNodes(data, flavor)[0];
      }
      else if (data instanceof XULElement && data.localName == "tab" &&
               data.ownerDocument.defaultView instanceof ChromeWindow) {
        let uri = data.linkedBrowser.currentURI;
        let spec = uri ? uri.spec : "about:blank";
        let title = data.label;
        unwrapped = { uri: spec,
                      title: data.label,
                      type: PlacesUtils.TYPE_X_MOZ_URL};
      }
      else
        throw("bogus data was passed as a tab")

      let index = insertionPoint.index;

      
      
      
      let dragginUp = insertionPoint.itemId == unwrapped.parent &&
                      index < PlacesUtils.bookmarks.getItemIndex(unwrapped.id);
      if (index != -1 && dragginUp)
        index+= movedCount++;

      
      if (insertionPoint.isTag &&
          insertionPoint.orientation == Ci.nsITreeView.DROP_ON) {
        let uri = NetUtil.newURI(unwrapped.uri);
        let tagItemId = insertionPoint.itemId;
        if (PlacesUIUtils.useAsyncTransactions)
          transactions.push(PlacesTransactions.Tag({ uri: uri, tag: tagName }));
        else
          transactions.push(new PlacesTagURITransaction(uri, [tagItemId]));
      }
      else {
        
        
        if (!doCopy && !PlacesControllerDragHelper.canMoveUnwrappedNode(unwrapped)) {
          Components.utils.reportError("Tried to move an unmovable Places node, " +
                                       "reverting to a copy operation.");
          doCopy = true;
        }
        if (PlacesUIUtils.useAsyncTransactions) {
          transactions.push(
            PlacesUIUtils.getTransactionForData(unwrapped,
                                                flavor,
                                                parentGuid,
                                                index,
                                                doCopy));
        }
        else {
          transactions.push(PlacesUIUtils.makeTransaction(unwrapped,
                              flavor, insertionPoint.itemId,
                              index, doCopy));
        }
      }
    }

    if (PlacesUIUtils.useAsyncTransactions) {
      yield PlacesTransactions.batch(transactions);
    }
    else {
      let txn = new PlacesAggregatedTransaction("DropItems", transactions);
      PlacesUtils.transactionManager.doTransaction(txn);
    }
  }),

  




  disallowInsertion: function(aContainer) {
    NS_ASSERT(aContainer, "empty container");
    
    return !PlacesUtils.nodeIsTagQuery(aContainer) &&
           (!PlacesUtils.nodeIsFolder(aContainer) ||
            PlacesUIUtils.isContentsReadOnly(aContainer));
  }
};


XPCOMUtils.defineLazyServiceGetter(PlacesControllerDragHelper, "dragService",
                                   "@mozilla.org/widget/dragservice;1",
                                   "nsIDragService");

function goUpdatePlacesCommands() {
  
  var placesController = doGetPlacesControllerForCommand("placesCmd_open");
  function updatePlacesCommand(aCommand) {
    goSetCommandEnabled(aCommand, placesController &&
                                  placesController.isCommandEnabled(aCommand));
  }

  updatePlacesCommand("placesCmd_open");
  updatePlacesCommand("placesCmd_open:window");
  updatePlacesCommand("placesCmd_open:privatewindow");
  updatePlacesCommand("placesCmd_open:tab");
  updatePlacesCommand("placesCmd_new:folder");
  updatePlacesCommand("placesCmd_new:bookmark");
  updatePlacesCommand("placesCmd_new:separator");
  updatePlacesCommand("placesCmd_show:info");
  updatePlacesCommand("placesCmd_moveBookmarks");
  updatePlacesCommand("placesCmd_reload");
  updatePlacesCommand("placesCmd_sortBy:name");
  updatePlacesCommand("placesCmd_cut");
  updatePlacesCommand("placesCmd_copy");
  updatePlacesCommand("placesCmd_paste");
  updatePlacesCommand("placesCmd_delete");
}

function doGetPlacesControllerForCommand(aCommand)
{
  
  
  let popupNode;
  try {
    popupNode = document.popupNode;
  } catch (e) {
    
    return null;
  }
  if (popupNode) {
    let view = PlacesUIUtils.getViewForNode(popupNode);
    if (view && view._contextMenuShown)
      return view.controllers.getControllerForCommand(aCommand);
  }

  
  
  let controller = top.document.commandDispatcher
                      .getControllerForCommand(aCommand);
  if (controller)
    return controller;

  return null;
}

function goDoPlacesCommand(aCommand)
{
  let controller = doGetPlacesControllerForCommand(aCommand);
  if (controller && controller.isCommandEnabled(aCommand))
    controller.doCommand(aCommand);
}

