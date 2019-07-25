







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");


const ORGANIZER_ROOT_BOOKMARKS = "place:folder=BOOKMARKS_MENU&excludeItems=1&queryType=1";
const ORGANIZER_SUBSCRIPTIONS_QUERY = "place:annotation=livemark%2FfeedURI";


const RELOAD_ACTION_NOTHING = 0;

const RELOAD_ACTION_INSERT = 1;

const RELOAD_ACTION_REMOVE = 2;


const RELOAD_ACTION_MOVE = 3;



const REMOVE_PAGES_CHUNKLEN = 300;



















function InsertionPoint(aItemId, aIndex, aOrientation, aIsTag,
                        aDropNearItemId) {
  this.itemId = aItemId;
  this._index = aIndex;
  this.orientation = aOrientation;
  this.isTag = aIsTag;
  this.dropNearItemId = aDropNearItemId;
}

InsertionPoint.prototype = {
  set index(val) {
    return this._index = val;
  },

  get index() {
    if (this.dropNearItemId > 0) {
      
      
      var index = PlacesUtils.bookmarks.getItemIndex(this.dropNearItemId);
      return this.orientation == Ci.nsITreeView.DROP_BEFORE ? index : index + 1;
    }
    return this._index;
  }
};





function PlacesController(aView) {
  this._view = aView;
  XPCOMUtils.defineLazyServiceGetter(this, "clipboard",
                                     "@mozilla.org/widget/clipboard;1",
                                     "nsIClipboard");
  XPCOMUtils.defineLazyGetter(this, "profileName", function () {
    return Services.dirsvc.get("ProfD", Ci.nsIFile).leafName;
  });
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
    switch (aCommand) {
    case "cmd_undo":
      return PlacesUIUtils.ptm.numberOfUndoItems > 0;
    case "cmd_redo":
      return PlacesUIUtils.ptm.numberOfRedoItems > 0;
    case "cmd_cut":
    case "placesCmd_cut":
      var nodes = this._view.selectedNodes;
      
      for (var i = 0; i < nodes.length; i++) {
        if (nodes[i].itemId == -1)
          return false;
      }
      
    case "cmd_delete":
    case "placesCmd_delete":
      return this._hasRemovableSelection(false);
    case "placesCmd_deleteDataHost":
      return this._hasRemovableSelection(false) &&
        !PlacesUIUtils.privateBrowsing.privateBrowsingEnabled;
    case "placesCmd_moveBookmarks":
      return this._hasRemovableSelection(true);
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
    case "placesCmd_open:tab":
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.nodeIsURI(selectedNode);
    case "placesCmd_new:folder":
    case "placesCmd_new:livemark":
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
      if (selectedNode &&
          PlacesUtils.getConcreteItemId(selectedNode) != -1  &&
          !PlacesUtils.nodeIsLivemarkItem(selectedNode))
        return true;
      return false;
    case "placesCmd_reload":
      
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.nodeIsLivemarkContainer(selectedNode);
    case "placesCmd_sortBy:name":
      var selectedNode = this._view.selectedNode;
      return selectedNode &&
             PlacesUtils.nodeIsFolder(selectedNode) &&
             !PlacesUtils.nodeIsReadOnly(selectedNode) &&
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
      PlacesUIUtils.ptm.undoTransaction();
      break;
    case "cmd_redo":
      PlacesUIUtils.ptm.redoTransaction();
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
      this.paste();
      break;
    case "cmd_delete":
    case "placesCmd_delete":
      this.remove("Remove Selection");
      break;
    case "placesCmd_deleteDataHost":
      var host;
      if (PlacesUtils.nodeIsHost(this._view.selectedNode)) {
        var queries = this._view.selectedNode.getQueries();
        host = queries[0].domain;
      }
      else
        host = NetUtil.newURI(this._view.selectedNode.uri).host;
      PlacesUIUtils.privateBrowsing.removeDataFromDomain(host);
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
    case "placesCmd_open:tab":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "tab", this._view);
      break;
    case "placesCmd_new:folder":
      this.newItem("folder");
      break;
    case "placesCmd_new:bookmark":
      this.newItem("bookmark");
      break;
    case "placesCmd_new:livemark":
      this.newItem("livemark");
      break;
    case "placesCmd_new:separator":
      this.newSeparator();
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
      this.sortFolderByName();
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


  











  _hasRemovableSelection: function PC__hasRemovableSelection(aIsMoveCommand) {
    var ranges = this._view.removableSelectionRanges;
    if (!ranges.length)
      return false;

    var root = this._view.result.root;

    for (var j = 0; j < ranges.length; j++) {
      var nodes = ranges[j];
      for (var i = 0; i < nodes.length; ++i) {
        
        if (nodes[i] == root)
          return false;

        if (PlacesUtils.nodeIsFolder(nodes[i]) &&
            !PlacesControllerDragHelper.canMoveNode(nodes[i]))
          return false;

        
        
        
        
        
        
        
        
        
        var parent = nodes[i].parent || root;
        if (PlacesUtils.isReadonlyFolder(parent))
          return false;
      }
    }

    return true;
  },

  


  _canInsert: function PC__canInsert(isPaste) {
    var ip = this._view.insertionPoint;
    return ip != null && (isPaste || ip.isTag != true);
  },

  


  rootNodeIsSelected: function PC_rootNodeIsSelected() {
    var nodes = this._view.selectedNodes;
    var root = this._view.result.root;
    for (var i = 0; i < nodes.length; ++i) {
      if (nodes[i] == root)
        return true;
    }

    return false;
  },

  







  _isClipboardDataPasteable: function PC__isClipboardDataPasteable() {
    
    

    var flavors = PlacesControllerDragHelper.placesFlavors;
    var clipboard = this.clipboard;
    var hasPlacesData =
      clipboard.hasDataMatchingFlavors(flavors, flavors.length,
                                       Ci.nsIClipboard.kGlobalClipboard);
    if (hasPlacesData)
      return this._view.insertionPoint != null;

    
    
    var xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);

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
    var root = this._view.result.root;
    var nodes = this._view.selectedNodes;
    if (nodes.length == 0)
      nodes.push(root); 

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
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_VISIT:
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FULL_VISIT:
          nodeData["link"] = true;
          uri = NetUtil.newURI(node.uri);
          if (PlacesUtils.nodeIsBookmark(node)) {
            nodeData["bookmark"] = true;
            PlacesUtils.nodeIsTagQuery(node.parent)

            var parentNode = node.parent;
            if (parentNode) {
              if (PlacesUtils.nodeIsTagQuery(parentNode))
                nodeData["tagChild"] = true;
              else if (PlacesUtils.nodeIsLivemarkContainer(parentNode))
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
    if (selectiontype == "multiple" && aMetaData.length == 1)
      return false;
    if (selectiontype == "single" && aMetaData.length != 1)
      return false;

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
    if (selectionAttr) {
      if (selectionAttr == "any")
        return true;

      var showRules = selectionAttr.split("|");
      var anyMatched = false;
      function metaDataNodeMatches(metaDataNode, rules) {
        for (var i=0; i < rules.length; i++) {
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
    }

    return !aMenuItem.hidden;
  },

  
































  buildContextMenu: function PC_buildContextMenu(aPopup) {
    var metadata = this._buildSelectionMetadata();
    var ip = this._view.insertionPoint;
    var noIp = !ip || ip.isTag;

    var separator = null;
    var visibleItemsBeforeSep = false;
    var anyVisible = false;
    for (var i = 0; i < aPopup.childNodes.length; ++i) {
      var item = aPopup.childNodes[i];
      if (item.localName != "menuseparator") {
        
        var hideIfNoIP = item.getAttribute("hideifnoinsertionpoint") == "true" &&
                         noIp && !(ip && ip.isTag && item.id == "placesContext_paste");
        var hideIfPB = item.getAttribute("hideifprivatebrowsing") == "true" &&
                       PlacesUIUtils.privateBrowsing.privateBrowsingEnabled;
        item.hidden = hideIfNoIP || hideIfPB ||
                      !this._shouldShowMenuItem(item, metadata);

        if (!item.hidden) {
          visibleItemsBeforeSep = true;
          anyVisible = true;

          
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

    
    if (anyVisible) {
      var openContainerInTabsItem = document.getElementById("placesContext_openContainer:tabs");
      if (!openContainerInTabsItem.hidden && this._view.selectedNode &&
          PlacesUtils.nodeIsContainer(this._view.selectedNode)) {
        openContainerInTabsItem.disabled =
          !PlacesUtils.hasChildURIs(this._view.selectedNode);
      }
      else {
        
        var openLinksInTabsItem = document.getElementById("placesContext_openLinks:tabs");
        openLinksInTabsItem.disabled = openLinksInTabsItem.hidden;
      }
    }

    return anyVisible;
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
    if (selectedNode && PlacesUtils.nodeIsLivemarkContainer(selectedNode))
      PlacesUtils.livemarks.reloadLivemarkFolder(selectedNode.itemId);
  },

  


  openSelectionInTabs: function PC_openLinksInTabs(aEvent) {
    var node = this._view.selectedNode;
    if (node && PlacesUtils.nodeIsContainer(node))
      PlacesUIUtils.openContainerNodeInTabs(this._view.selectedNode, aEvent, this._view);
    else
      PlacesUIUtils.openURINodesInTabs(this._view.selectedNodes, aEvent, this._view);
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

  


  newSeparator: function PC_newSeparator() {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    var txn = PlacesUIUtils.ptm.createSeparator(ip.itemId, ip.index);
    PlacesUIUtils.ptm.doTransaction(txn);
    
    var insertedNodeId = PlacesUtils.bookmarks
                                    .getIdForItemAt(ip.itemId, ip.index);
    this._view.selectItems([insertedNodeId], false);
  },

  


  moveSelectedBookmarks: function PC_moveBookmarks() {
    window.openDialog("chrome://browser/content/places/moveBookmarks.xul",
                      "", "chrome, modal",
                      this._view.selectedNodes);
  },

  


  sortFolderByName: function PC_sortFolderByName() {
    var itemId = PlacesUtils.getConcreteItemId(this._view.selectedNode);
    var txn = PlacesUIUtils.ptm.sortFolderByName(itemId);
    PlacesUIUtils.ptm.doTransaction(txn);
  },

  









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
        transactions.push(PlacesUIUtils.ptm.untagURI(uri, [tagItemId]));
      }
      else if (PlacesUtils.nodeIsTagQuery(node) && node.parent &&
               PlacesUtils.nodeIsQuery(node.parent) &&
               PlacesUtils.asQuery(node.parent).queryOptions.resultType ==
                 Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY) {
        
        
        
        
        var tag = node.title;
        var URIs = PlacesUtils.tagging.getURIsForTag(tag);
        for (var j = 0; j < URIs.length; j++)
          transactions.push(PlacesUIUtils.ptm.untagURI(URIs[j], [tag]));
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
        transactions.push(PlacesUIUtils.ptm.removeItem(node.itemId));
      }
    }
  },

  




  _removeRowsFromBookmarks: function PC__removeRowsFromBookmarks(txnName) {
    var ranges = this._view.removableSelectionRanges;
    var transactions = [];
    var removedFolders = [];

    for (var i = 0; i < ranges.length; i++)
      this._removeRange(ranges[i], transactions, removedFolders);

    if (transactions.length > 0) {
      var txn = PlacesUIUtils.ptm.aggregateTransactions(txnName, transactions);
      PlacesUIUtils.ptm.doTransaction(txn);
    }
  },

  




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
        yield;
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

  





  remove: function PC_remove(aTxnName) {
    if (!this._hasRemovableSelection(false))
      return;

    NS_ASSERT(aTxnName !== undefined, "Must supply Transaction Name");

    var root = this._view.result.root;

    if (PlacesUtils.nodeIsFolder(root))
      this._removeRowsFromBookmarks(aTxnName);
    else if (PlacesUtils.nodeIsQuery(root)) {
      var queryType = PlacesUtils.asQuery(root).queryOptions.queryType;
      if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS)
        this._removeRowsFromBookmarks(aTxnName);
      else if (queryType == Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY)
        this._removeRowsFromHistory();
      else
        NS_ASSERT(false, "implement support for QUERY_TYPE_UNIFIED");
    }
    else
      NS_ASSERT(false, "unexpected root");
  },

  





  setDataTransfer: function PC_setDataTransfer(aEvent) {
    let dt = aEvent.dataTransfer;
    let doCopy = ["copyLink", "copy", "link"].indexOf(dt.effectAllowed) != -1;

    let result = this._view.result;
    let didSuppressNotifications = result.suppressNotifications;
    if (!didSuppressNotifications)
      result.suppressNotifications = true;

    try {
      let nodes = this._view.draggableSelection;
      for (let i = 0; i < nodes.length; ++i) {
        var node = nodes[i];

        function addData(type, index, overrideURI) {
          let wrapNode = PlacesUtils.wrapNode(node, type, overrideURI, doCopy);
          dt.mozSetDataAt(type, wrapNode, index);
        }

        function addURIData(index, overrideURI) {
          addData(PlacesUtils.TYPE_X_MOZ_URL, index, overrideURI);
          addData(PlacesUtils.TYPE_UNICODE, index, overrideURI);
          addData(PlacesUtils.TYPE_HTML, index, overrideURI);
        }

        
        
        addData(PlacesUtils.TYPE_X_MOZ_PLACE, i);

        
        if (PlacesUtils.nodeIsLivemarkContainer(node))
          addURIData(i, PlacesUtils.livemarks.getFeedURI(node.itemId).spec);
        else if (node.uri)
          addURIData(i);
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

      let overrideURI = PlacesUtils.nodeIsLivemarkContainer(node) ?
        PlacesUtils.livemarks.getFeedURI(node.itemId).spec : null;
      let resolveShortcuts = !PlacesControllerDragHelper.canMoveNode(node);

      contents.forEach(function (content) {
        content.entries.push(
          PlacesUtils.wrapNode(node, content.type, overrideURI, resolveShortcuts)
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

  


  paste: function PC_paste() {
    
    let ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    let action = this.clipboardAction;

    let xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    
    
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

    let transactions = [];
    let insertionIndex = ip.index;
    for (let i = 0; i < items.length; ++i) {
      if (ip.isTag) {
        
        
        transactions.push(
          new PlacesTagURITransaction(NetUtil.newURI(items[i].uri),
                                      [ip.itemId])
        );
        continue;
      }

      
      
      if (ip.index != PlacesUtils.bookmarks.DEFAULT_INDEX)
        insertionIndex = ip.index + i;

      transactions.push(
        PlacesUIUtils.makeTransaction(items[i], type, ip.itemId,
                                      insertionIndex, action == "copy")
      );
    }
 
    PlacesUtils.transactionManager.doTransaction(
      new PlacesAggregatedTransaction("Paste", transactions)
    );

    
    if (action == "cut") {
      this._clearClipboard();
    }

    
    let insertedNodeIds = [];
    for (let i = 0; i < transactions.length; ++i) {
      insertedNodeIds.push(
        PlacesUtils.bookmarks.getIdForItemAt(ip.itemId, ip.index + i)
      );
    }
    if (insertedNodeIds.length > 0)
      this._view.selectItems(insertedNodeIds, false);
  }
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
      if (this.GENERIC_VIEW_DROP_TYPES.indexOf(aFlavors[i]) != -1)
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
           /^place:/.test(dragged.uri)))
        return false;

      
      
      if (dragged.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER ||
          /^place:/.test(dragged.uri)) {
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


  






  canMoveNode:
  function PCDH_canMoveNode(aNode) {
    
    if (!aNode.parent)
      return false;

    let parentId = PlacesUtils.getConcreteItemId(aNode.parent);
    let concreteId = PlacesUtils.getConcreteItemId(aNode);

    
    if (PlacesUtils.nodeIsTagQuery(aNode.parent))
      return false;

    
    if (PlacesUtils.nodeIsReadOnly(aNode.parent))
      return false;

    
    if (PlacesUtils.nodeIsContainer(aNode) &&
        !this.canMoveContainer(aNode.itemId, parentId))
      return false;

    return true;
  },

  








  canMoveContainer:
  function PCDH_canMoveContainer(aId, aParentId) {
    if (aId == -1)
      return false;

    
    const ROOTS = [PlacesUtils.placesRootId, PlacesUtils.bookmarksMenuFolderId,
                   PlacesUtils.tagsFolderId, PlacesUtils.unfiledBookmarksFolderId,
                   PlacesUtils.toolbarFolderId];
    if (ROOTS.indexOf(aId) != -1)
      return false;

    
    if (aParentId == null || aParentId == -1)
      aParentId = PlacesUtils.bookmarks.getFolderIdForItem(aId);

    if (PlacesUtils.bookmarks.getFolderReadonly(aParentId))
      return false;

    return true;
  },

  




  onDrop: function PCDH_onDrop(insertionPoint, dt) {
    let doCopy = ["copy", "link"].indexOf(dt.dropEffect) != -1;

    let transactions = [];
    let dropCount = dt.mozItemCount;
    let movedCount = 0;
    for (let i = 0; i < dropCount; ++i) {
      let flavor = this.getFirstValidFlavor(dt.mozTypesAt(i));
      if (!flavor)
        return false;

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
        transactions.push(PlacesUIUtils.ptm.tagURI(uri,[tagItemId]));
      }
      else {
        transactions.push(PlacesUIUtils.makeTransaction(unwrapped,
                          flavor, insertionPoint.itemId,
                          index, doCopy));
      }
    }

    let txn = PlacesUIUtils.ptm.aggregateTransactions("DropItems", transactions);
    PlacesUIUtils.ptm.doTransaction(txn);
  },

  




  disallowInsertion: function(aContainer) {
    NS_ASSERT(aContainer, "empty container");
    
    if (PlacesUtils.nodeIsTagQuery(aContainer))
      return false;
    
    return (!PlacesUtils.nodeIsFolder(aContainer) ||
             PlacesUtils.nodeIsReadOnly(aContainer));
  },

  placesFlavors: [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                  PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                  PlacesUtils.TYPE_X_MOZ_PLACE],

  
  GENERIC_VIEW_DROP_TYPES: [PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER,
                            PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR,
                            PlacesUtils.TYPE_X_MOZ_PLACE,
                            PlacesUtils.TYPE_X_MOZ_URL,
                            TAB_DROP_TYPE,
                            PlacesUtils.TYPE_UNICODE],
};


XPCOMUtils.defineLazyServiceGetter(PlacesControllerDragHelper, "dragService",
                                   "@mozilla.org/widget/dragservice;1",
                                   "nsIDragService");

function goUpdatePlacesCommands() {
  
  var placesController = doGetPlacesControllerForCommand("placesCmd_open");
  if (!placesController)
    return;

  function updatePlacesCommand(aCommand) {
    goSetCommandEnabled(aCommand, placesController.isCommandEnabled(aCommand));
  }

  updatePlacesCommand("placesCmd_open");
  updatePlacesCommand("placesCmd_open:window");
  updatePlacesCommand("placesCmd_open:tab");
  updatePlacesCommand("placesCmd_new:folder");
  updatePlacesCommand("placesCmd_new:bookmark");
  updatePlacesCommand("placesCmd_new:livemark");
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
  
  
  let popupNode = document.popupNode;
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

