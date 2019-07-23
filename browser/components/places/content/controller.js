








































const ORGANIZER_ROOT_BOOKMARKS = "place:folder=BOOKMARKS_MENU&excludeItems=1&queryType=1";
const ORGANIZER_SUBSCRIPTIONS_QUERY = "place:annotation=livemark%2FfeedURI";


const RELOAD_ACTION_NOTHING = 0;

const RELOAD_ACTION_INSERT = 1;

const RELOAD_ACTION_REMOVE = 2;


const RELOAD_ACTION_MOVE = 3;





const REMOVE_PAGES_CHUNKLEN = 300;




const REMOVE_PAGES_MAX_SINGLEREMOVES = 10;



















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
}

PlacesController.prototype = {
  


  _view: null,

  isCommandEnabled: function PC_isCommandEnabled(aCommand) {
    switch (aCommand) {
    case "cmd_undo":
      return PlacesUIUtils.ptm.numberOfUndoItems > 0;
    case "cmd_redo":
      return PlacesUIUtils.ptm.numberOfRedoItems > 0;
    case "cmd_cut":
    case "placesCmd_cut":
      var nodes = this._view.getSelectionNodes();
      
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
        var rootNode = this._view.getResultNode();
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
             !asQuery(this._view.getResultNode()).queryOptions.excludeItems &&
             this._view.getResult().sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_show:info":
      var selectedNode = this._view.selectedNode;
      if (selectedNode &&
          PlacesUtils.getConcreteItemId(selectedNode) != -1  &&
          !PlacesUtils.nodeIsLivemarkItem(selectedNode))
        return true;
      return false;
    case "placesCmd_reloadMicrosummary":
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.nodeIsBookmark(selectedNode) &&
             PlacesUIUtils.microsummaries.hasMicrosummary(selectedNode.itemId);
    case "placesCmd_reload":
      
      var selectedNode = this._view.selectedNode;
      return selectedNode && PlacesUtils.nodeIsLivemarkContainer(selectedNode);
    case "placesCmd_sortBy:name":
      var selectedNode = this._view.selectedNode;
      return selectedNode &&
             PlacesUtils.nodeIsFolder(selectedNode) &&
             !PlacesUtils.nodeIsReadOnly(selectedNode) &&
             this._view.getResult().sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_createBookmark":
      var node = this._view.selectedNode;
      return node && PlacesUtils.nodeIsURI(node) && node.itemId == -1;
    default:
      return false;
    }
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
        host = PlacesUtils._uri(this._view.selectedNode.uri).host;
      PlacesUIUtils.privateBrowsing.removeDataFromDomain(host);
      break;
    case "cmd_selectAll":
      this.selectAll();
      break;
    case "placesCmd_open":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "current");
      break;
    case "placesCmd_open:window":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "window");
      break;
    case "placesCmd_open:tab":
      PlacesUIUtils.openNodeIn(this._view.selectedNode, "tab");
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
    case "placesCmd_reloadMicrosummary":
      this.reloadSelectedMicrosummary();
      break;
    case "placesCmd_sortBy:name":
      this.sortFolderByName();
      break;
    case "placesCmd_createBookmark":
      var node = this._view.selectedNode;
      PlacesUIUtils.showMinimalAddBookmarkUI(PlacesUtils._uri(node.uri), node.title);
      break;
    }
  },

  onEvent: function PC_onEvent(eventName) { },

  
  











  _hasRemovableSelection: function PC__hasRemovableSelection(aIsMoveCommand) {
    var ranges = this._view.getRemovableSelectionRanges();
    if (!ranges.length)
      return false;

    var root = this._view.getResultNode();

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
    var nodes = this._view.getSelectionNodes();
    var root = this._view.getResultNode();
    for (var i = 0; i < nodes.length; ++i) {
      if (nodes[i] == root)
        return true;      
    }

    return false;
  },

  







  _isClipboardDataPasteable: function PC__isClipboardDataPasteable() {
    
    

    var flavors = PlacesControllerDragHelper.placesFlavors;
    var clipboard = PlacesUIUtils.clipboard;
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
    var root = this._view.getResultNode();
    var nodes = this._view.getSelectionNodes();
    if (nodes.length == 0)
      nodes.push(root); 

    for (var i=0; i < nodes.length; i++) {
      var nodeData = {};
      var node = nodes[i];
      var nodeType = node.type;
      var uri = null;

      
      
      switch(nodeType) {
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY:
          nodeData["query"] = true;
          if (node.parent) {
            switch (asQuery(node.parent).queryOptions.resultType) {
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
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_DYNAMIC_CONTAINER:
          nodeData["dynamiccontainer"] = true;
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
          uri = PlacesUtils._uri(node.uri);
          if (PlacesUtils.nodeIsBookmark(node)) {
            nodeData["bookmark"] = true;
            PlacesUtils.nodeIsTagQuery(node.parent)
            var mss = PlacesUIUtils.microsummaries;
            if (mss.hasMicrosummary(node.itemId))
              nodeData["microsummary"] = true;

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
        var names = PlacesUtils.annotations.getPageAnnotationNames(uri);
        for (var j = 0; j < names.length; ++j)
          nodeData[names[j]] = true;
      }

      
      if (node.itemId != -1) {
        names = PlacesUtils.annotations
                           .getItemAnnotationNames(node.itemId);
        for (j = 0; j < names.length; ++j)
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
      for (var i = 0; i < aMetaData.length; ++i) {
        for (var j=0; j < forceHideRules.length; ++j) {
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

    PlacesUIUtils.showItemProperties(itemId, itemType,
                                     isRootItem );
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

  


  reloadSelectedMicrosummary: function PC_reloadSelectedMicrosummary() {
    var selectedNode = this._view.selectedNode;
    var mss = PlacesUIUtils.microsummaries;
    if (mss.hasMicrosummary(selectedNode.itemId))
      mss.refreshMicrosummary(selectedNode.itemId);
  },

  


  _confirmOpenTabs: function(numTabsToOpen) {
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
        var strings = document.getElementById("placeBundle");
        const BRANDING_BUNDLE_URI = "chrome://branding/locale/brand.properties";
        var brandShortName = Cc["@mozilla.org/intl/stringbundle;1"].
                             getService(Ci.nsIStringBundleService).
                             createBundle(BRANDING_BUNDLE_URI).
                             GetStringFromName("brandShortName");
       
        var buttonPressed = promptService.confirmEx(window,
          PlacesUIUtils.getString("tabs.openWarningTitle"),
          PlacesUIUtils.getFormattedString(messageKey, 
            [numTabsToOpen, brandShortName]),
          (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0)
          + (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
          PlacesUIUtils.getString(openKey),
          null, null,
          PlacesUIUtils.getFormattedString("tabs.openWarningPromptMeBranded",
            [brandShortName]),
          warnOnOpen);

         reallyOpen = (buttonPressed == 0);
         
         if (reallyOpen && !warnOnOpen.value)
           pref.setBoolPref(kWarnOnOpenPref, false);
      }
    }
    return reallyOpen;
  },

  


  openSelectionInTabs: function PC_openLinksInTabs(aEvent) {
    var node = this._view.selectedNode;
    if (node && PlacesUtils.nodeIsContainer(node))
      PlacesUIUtils.openContainerNodeInTabs(this._view.selectedNode, aEvent);
    else
      PlacesUIUtils.openURINodesInTabs(this._view.getSelectionNodes(), aEvent);
  },

  





  newItem: function PC_newItem(aType) {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    var performed = false;
    if (aType == "bookmark")
      performed = PlacesUIUtils.showAddBookmarkUI(null, null, null, ip);
    else if (aType == "livemark")
      performed = PlacesUIUtils.showAddLivemarkUI(null, null, null, null, ip);
    else 
      performed = PlacesUIUtils.showAddFolderUI(null, ip);

    if (performed) {
      
      var insertedNodeId = PlacesUtils.bookmarks
                                      .getIdForItemAt(ip.itemId, ip.index);
      this._view.selectItems([insertedNodeId], false);
    }
  },


  



  newFolder: function PC_newFolder() {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    var performed = false;
    performed = PlacesUIUtils.showAddFolderUI(null, ip);
    if (performed) {
      
      var insertedNodeId = PlacesUtils.bookmarks
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
                      this._view.getSelectionNodes());
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
        var uri = PlacesUtils._uri(node.uri);
        transactions.push(PlacesUIUtils.ptm.untagURI(uri, [tagItemId]));
      }
      else if (PlacesUtils.nodeIsTagQuery(node) && node.parent &&
               PlacesUtils.nodeIsQuery(node.parent) &&
               asQuery(node.parent).queryOptions.resultType ==
                 Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY) {
        
        
        
        
        var tag = node.title;
        var URIs = PlacesUtils.tagging.getURIsForTag(tag);
        for (var j = 0; j < URIs.length; j++)
          transactions.push(PlacesUIUtils.ptm.untagURI(URIs[j], [tag]));
      }
      else if (PlacesUtils.nodeIsURI(node) &&
               PlacesUtils.nodeIsQuery(node.parent) &&
               asQuery(node.parent).queryOptions.queryType ==
                 Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        
        var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
        bhist.removePage(PlacesUtils._uri(node.uri));
        
      }
      else if (node.itemId == -1 &&
               PlacesUtils.nodeIsQuery(node) &&
               asQuery(node).queryOptions.queryType ==
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
    var ranges = this._view.getRemovableSelectionRanges();
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
    
    
    var nodes = this._view.getSelectionNodes();
    var URIs = [];
    var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
    var root = this._view.getResultNode();

    for (var i = 0; i < nodes.length; ++i) {
      var node = nodes[i];
      if (PlacesUtils.nodeIsURI(node)) {
        var uri = PlacesUtils._uri(node.uri);
        
        if (URIs.indexOf(uri) < 0) {
          URIs.push(uri);
        }
      }
      else if (PlacesUtils.nodeIsQuery(node) &&
               asQuery(node).queryOptions.queryType ==
                 Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY)
        this._removeHistoryContainer(node);
    }

    
    
    if (URIs.length > REMOVE_PAGES_MAX_SINGLEREMOVES) {
      
      for (var i = 0; i < URIs.length; i += REMOVE_PAGES_CHUNKLEN) {
        var URIslice = URIs.slice(i, i + REMOVE_PAGES_CHUNKLEN);
        
        
        bhist.removePages(URIslice, URIslice.length,
                          (i + REMOVE_PAGES_CHUNKLEN) >= URIs.length);
      }
    }
    else {
      
      
      for (var i = 0; i < URIs.length; ++i)
        bhist.removePage(URIs[i]);
    }
  },

  




  _removeHistoryContainer: function PC_removeHistoryContainer(aContainerNode) {
    var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
    if (PlacesUtils.nodeIsHost(aContainerNode)) {
      
      bhist.removePagesFromHost(aContainerNode.title, true);
    }
    else if (PlacesUtils.nodeIsDay(aContainerNode)) {
      
      var query = aContainerNode.getQueries()[0];
      var beginTime = query.beginTime;
      var endTime = query.endTime;
      NS_ASSERT(query && beginTime && endTime,
                "A valid date container query should exist!");
      
      
      
      
      bhist.removePagesByTimeframe(beginTime+1, endTime);
    }
  },

  





  remove: function PC_remove(aTxnName) {
    if (!this._hasRemovableSelection(false))
      return;

    NS_ASSERT(aTxnName !== undefined, "Must supply Transaction Name");

    var root = this._view.getResultNode();

    if (PlacesUtils.nodeIsFolder(root)) 
      this._removeRowsFromBookmarks(aTxnName);
    else if (PlacesUtils.nodeIsQuery(root)) {
      var queryType = asQuery(root).queryOptions.queryType;
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
    var dt = aEvent.dataTransfer;
    var doCopy = ["copyLink", "copy", "link"].indexOf(dt.effectAllowed) != -1;

    var result = this._view.getResult();
    var oldViewer = result.viewer;
    try {
      result.viewer = null;
      var nodes = this._view.getDraggableSelection();

      for (var i = 0; i < nodes.length; ++i) {
        var node = nodes[i];

        function addData(type, index, overrideURI) {
          var wrapNode = PlacesUtils.wrapNode(node, type, overrideURI, doCopy);
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
      if (oldViewer)
        result.viewer = oldViewer;
    }
  },

  


  copy: function PC_copy() {
    var result = this._view.getResult();
    var oldViewer = result.viewer;
    try {
      result.viewer = null;
      var nodes = this._view.getSelectionNodes();

      var xferable =  Cc["@mozilla.org/widget/transferable;1"].
                      createInstance(Ci.nsITransferable);
      var foundFolder = false, foundLink = false;
      var copiedFolders = [];
      var placeString, mozURLString, htmlString, unicodeString;
      placeString = mozURLString = htmlString = unicodeString = "";

      for (var i = 0; i < nodes.length; ++i) {
        var node = nodes[i];
        if (this._shouldSkipNode(node, copiedFolders))
          continue;
        if (PlacesUtils.nodeIsFolder(node))
          copiedFolders.push(node);
        
        function generateChunk(type, overrideURI) {
          var suffix = i < (nodes.length - 1) ? NEWLINE : "";
          var uri = overrideURI;
        
          if (PlacesUtils.nodeIsLivemarkContainer(node))
            uri = PlacesUtils.livemarks.getFeedURI(node.itemId).spec

          mozURLString += (PlacesUtils.wrapNode(node, PlacesUtils.TYPE_X_MOZ_URL,
                                                 uri) + suffix);
          unicodeString += (PlacesUtils.wrapNode(node, PlacesUtils.TYPE_UNICODE,
                                                 uri) + suffix);
          htmlString += (PlacesUtils.wrapNode(node, PlacesUtils.TYPE_HTML,
                                                 uri) + suffix);

          var placeSuffix = i < (nodes.length - 1) ? "," : "";
          var resolveShortcuts = !PlacesControllerDragHelper.canMoveNode(node);
          return PlacesUtils.wrapNode(node, type, overrideURI, resolveShortcuts) + placeSuffix;
        }

        
        placeString += generateChunk(PlacesUtils.TYPE_X_MOZ_PLACE);
      }

      function addData(type, data) {
        xferable.addDataFlavor(type);
        xferable.setTransferData(type, PlacesUIUtils._wrapString(data), data.length * 2);
      }
      
      
      if (placeString)
        addData(PlacesUtils.TYPE_X_MOZ_PLACE, placeString);
      if (mozURLString)
        addData(PlacesUtils.TYPE_X_MOZ_URL, mozURLString);
      if (unicodeString)
        addData(PlacesUtils.TYPE_UNICODE, unicodeString);
      if (htmlString)
        addData(PlacesUtils.TYPE_HTML, htmlString);

      if (placeString || unicodeString || htmlString || mozURLString) {
        PlacesUIUtils.clipboard.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
      }
    }
    finally {
      if (oldViewer)
        result.viewer = oldViewer;
    }
  },

  


  cut: function PC_cut() {
    this.copy();
    this.remove("Cut Selection");
  },

  


  paste: function PC_paste() {
    
    
    
    
    
    

    






    function makeXferable(types) {
      var xferable = 
          Cc["@mozilla.org/widget/transferable;1"].
          createInstance(Ci.nsITransferable);
      for (var i = 0; i < types.length; ++i) 
        xferable.addDataFlavor(types[i]);
      return xferable;
    }

    var clipboard = PlacesUIUtils.clipboard;

    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    





    function getTransactions(types) {
      var xferable = makeXferable(types);
      clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);

      var data = { }, type = { };
      try {
        xferable.getAnyTransferData(type, data, { });
        data = data.value.QueryInterface(Ci.nsISupportsString).data;
        var items = PlacesUtils.unwrapNodes(data, type.value);
        var transactions = [];
        var index = ip.index;
        for (var i = 0; i < items.length; ++i) {
          var txn;
          if (ip.isTag) {
            var uri = PlacesUtils._uri(items[i].uri);
            txn = PlacesUIUtils.ptm.tagURI(uri, [ip.itemId]);
          } 
          else {
            
            
            
            if (ip.index > -1)
              index = ip.index + i;
            txn = PlacesUIUtils.makeTransaction(items[i], type.value,
                                                ip.itemId, index, true);
          }
          transactions.push(txn);
        }
        return transactions;
      }
      catch (e) {
        
        
        
        
        
      }
      return [];
    }

    
    
    var transactions = getTransactions([PlacesUtils.TYPE_X_MOZ_PLACE,
                                        PlacesUtils.TYPE_X_MOZ_URL, 
                                        PlacesUtils.TYPE_UNICODE]);
    var txn = PlacesUIUtils.ptm.aggregateTransactions("Paste", transactions);
    PlacesUIUtils.ptm.doTransaction(txn);

    
    var insertedNodeIds = [];
    for (var i = 0; i < transactions.length; ++i)
      insertedNodeIds.push(PlacesUtils.bookmarks
                                      .getIdForItemAt(ip.itemId, ip.index + i));
    if (insertedNodeIds.length > 0)
      this._view.selectItems(insertedNodeIds, false);
  }
};







var PlacesControllerDragHelper = {
  


  currentDropTarget: null,

  





  currentDataTransfer: null,

  








  draggingOverChildNode: function PCDH_draggingOverChildNode(node) {
    var currentNode = this.currentDropTarget;
    while (currentNode) {
      if (currentNode == node)
        return true;
      currentNode = currentNode.parentNode;
    }
    return false;
  },

  


  getSession: function PCDH__getSession() {
    var dragService = Cc["@mozilla.org/widget/dragservice;1"].
                      getService(Ci.nsIDragService);
    return dragService.getCurrentSession();
  },

  




  getFirstValidFlavor: function PCDH_getFirstValidFlavor(aFlavors) {
    for (var i = 0; i < aFlavors.length; i++) {
      if (this.GENERIC_VIEW_DROP_TYPES.indexOf(aFlavors[i]) != -1)
        return aFlavors[i];
    }
    return null;
  },

  





  canDrop: function PCDH_canDrop(ip) {
    var dt = this.currentDataTransfer;
    var dropCount = dt.mozItemCount;

    
    for (var i = 0; i < dropCount; i++) {
      var flavor = this.getFirstValidFlavor(dt.mozTypesAt(i));
      if (!flavor)
        return false;

      var data = dt.mozGetDataAt(flavor, i);

      
      
      
      
      
      
      
      
      
      
      if (flavor == TAB_DROP_TYPE)
        continue;

      try {
        var dragged = PlacesUtils.unwrapNodes(data, flavor)[0];
      } catch (e) {
        return false;
      }

      
      if (ip.isTag && ip.orientation == Ci.nsITreeView.DROP_ON &&
          dragged.type != PlacesUtils.TYPE_X_MOZ_URL &&
          (dragged.type != PlacesUtils.TYPE_X_MOZ_PLACE ||
           /^place:/.test(dragged.uri)))
        return false;

      
      
      if (dragged.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER ||
          /^place:/.test(dragged.uri)) {
        var parentId = ip.itemId;
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

    var parentId = PlacesUtils.getConcreteItemId(aNode.parent);
    var concreteId = PlacesUtils.getConcreteItemId(aNode);

    
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

  




  onDrop: function PCDH_onDrop(insertionPoint) {
    var dt = this.currentDataTransfer;
    var doCopy = ["copy", "link"].indexOf(dt.dropEffect) != -1;

    var transactions = [];
    var dropCount = dt.mozItemCount;
    var movedCount = 0;
    for (var i = 0; i < dropCount; ++i) {
      var flavor = this.getFirstValidFlavor(dt.mozTypesAt(i));
      if (!flavor)
        return false;

      var data = dt.mozGetDataAt(flavor, i);
      var unwrapped;
      if (flavor != TAB_DROP_TYPE) {
        
        unwrapped = PlacesUtils.unwrapNodes(data, flavor)[0];
      }
      else if (data instanceof XULElement && data.localName == "tab" &&
               data.ownerDocument.defaultView instanceof ChromeWindow) {
        var uri = data.linkedBrowser.currentURI;
        var spec = uri ? uri.spec : "about:blank";
        var title = data.label;
        unwrapped = { uri: spec,
                      title: data.label,
                      type: PlacesUtils.TYPE_X_MOZ_URL};
      }
      else
        throw("bogus data was passed as a tab")

      var index = insertionPoint.index;

      
      
      
      var dragginUp = insertionPoint.itemId == unwrapped.parent &&
                      index < PlacesUtils.bookmarks.getItemIndex(unwrapped.id);
      if (index != -1 && dragginUp)
        index+= movedCount++;

      
      if (insertionPoint.isTag &&
          insertionPoint.orientation == Ci.nsITreeView.DROP_ON) {
        var uri = PlacesUtils._uri(unwrapped.uri);
        var tagItemId = insertionPoint.itemId;
        transactions.push(PlacesUIUtils.ptm.tagURI(uri,[tagItemId]));
      }
      else {
        transactions.push(PlacesUIUtils.makeTransaction(unwrapped,
                          flavor, insertionPoint.itemId,
                          index, doCopy));
      }
    }

    var txn = PlacesUIUtils.ptm.aggregateTransactions("DropItems", transactions);
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

  


  get flavourSet() {
    delete this.flavourSet;
    var flavourSet = new FlavourSet();
    var acceptedDropFlavours = this.GENERIC_VIEW_DROP_TYPES;
    acceptedDropFlavours.forEach(flavourSet.appendFlavour, flavourSet);
    return this.flavourSet = flavourSet;
  }
};

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
  updatePlacesCommand("placesCmd_reloadMicrosummary");
  updatePlacesCommand("placesCmd_sortBy:name");
  updatePlacesCommand("placesCmd_cut");
  updatePlacesCommand("placesCmd_copy");
  updatePlacesCommand("placesCmd_paste");
  updatePlacesCommand("placesCmd_delete");
}

function doGetPlacesControllerForCommand(aCommand)
{
  var placesController = top.document.commandDispatcher
                            .getControllerForCommand(aCommand);
  if (!placesController) {
    
    
    var element = document.popupNode;
    while (element) {
      var isContextMenuShown = ("_contextMenuShown" in element) && element._contextMenuShown;
      
      if ((element.localName == "menupopup" || element.localName == "hbox") &&
          isContextMenuShown) {
        placesController = element.controllers.getControllerForCommand(aCommand);
        break;
      }
      element = element.parentNode;
    }
  }

  return placesController;
}

function goDoPlacesCommand(aCommand)
{
  var controller = doGetPlacesControllerForCommand(aCommand);
  if (controller && controller.isCommandEnabled(aCommand))
    controller.doCommand(aCommand);
}

