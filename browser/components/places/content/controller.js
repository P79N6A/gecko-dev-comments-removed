







































const ORGANIZER_ROOT_BOOKMARKS = "place:folder=2&excludeItems=1&queryType=1";
const ORGANIZER_SUBSCRIPTIONS_QUERY = "place:annotation=livemark%2FfeedURI";


const RELOAD_ACTION_NOTHING = 0;

const RELOAD_ACTION_INSERT = 1;

const RELOAD_ACTION_REMOVE = 2;


const RELOAD_ACTION_MOVE = 3;















function InsertionPoint(aItemId, aIndex, aOrientation) {
  this.itemId = aItemId;
  this.index = aIndex;
  this.orientation = aOrientation;
}
InsertionPoint.prototype.toString = function IP_toString() {
  return "[object InsertionPoint(folder:" + this.itemId + ",index:" + this.index + ",orientation:" + this.orientation + ")]";
};





function PlacesController(aView) {
  this._view = aView;
}

PlacesController.prototype = {
  


  _view: null,

  isCommandEnabled: function PC_isCommandEnabled(aCommand) {
    switch (aCommand) {
    case "cmd_undo":
      return PlacesUtils.ptm.numberOfUndoItems > 0;
    case "cmd_redo":
      return PlacesUtils.ptm.numberOfRedoItems > 0;
    case "cmd_cut":
    case "cmd_delete":
      return this._hasRemovableSelection(false);
    case "placesCmd_moveBookmarks":
      return this._hasRemovableSelection(true);
    case "cmd_copy":
      return this._view.hasSelection;
    case "cmd_paste":
      return this._canInsert() && this._isClipboardDataPasteable();
    case "cmd_selectAll":
      if (this._view.selType != "single") {
        var result = this._view.getResult();
        if (result) {
          var container = asContainer(result.root);
          if (container.childCount > 0);
            return true;
        }
      }
      return false;
    case "placesCmd_open":
    case "placesCmd_open:window":
    case "placesCmd_open:tab":
      return this._view.selectedURINode;
    case "placesCmd_new:folder":
    case "placesCmd_new:livemark":
      return this._canInsert();
    case "placesCmd_new:bookmark":
      return this._canInsert();
    case "placesCmd_new:separator":
      return this._canInsert() &&
             !asQuery(this._view.getResult().root).queryOptions.excludeItems &&
             this._view.getResult().sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_show:info":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;
        if (PlacesUtils.nodeIsFolder(selectedNode) ||
            (PlacesUtils.nodeIsBookmark(selectedNode) &&
            !PlacesUtils.nodeIsLivemarkItem(selectedNode)))
          return true;
      }
      return false;
    case "placesCmd_reloadMicrosummary":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;
        if (PlacesUtils.nodeIsBookmark(selectedNode)) {
          var mss = PlacesUtils.microsummaries;
          if (mss.hasMicrosummary(selectedNode.itemId))
            return true;
        }
      }
      return false;
    case "placesCmd_reload":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;

        
        if (PlacesUtils.nodeIsLivemarkContainer(selectedNode))
          return true;
      }
      return false;
    case "placesCmd_sortBy:name":
      var selectedNode = this._view.selectedNode;
      return selectedNode &&
             PlacesUtils.nodeIsFolder(selectedNode) &&
             !PlacesUtils.nodeIsReadOnly(selectedNode) &&
             this._view.getResult().sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
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
      PlacesUtils.ptm.undoTransaction();
      break;
    case "cmd_redo":
      PlacesUtils.ptm.redoTransaction();
      break;
    case "cmd_cut":
      this.cut();
      break;
    case "cmd_copy":
      this.copy();
      break;
    case "cmd_paste":
      this.paste();
      break;
    case "cmd_delete":
      this.remove("Remove Selection");
      break;
    case "cmd_selectAll":
      this.selectAll();
      break;
    case "placesCmd_open":
      this.openSelectedNodeIn("current");
      break;
    case "placesCmd_open:window":
      this.openSelectedNodeIn("window");
      break;
    case "placesCmd_open:tab":
      this.openSelectedNodeIn("tab");
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
    }
  },

  onEvent: function PC_onEvent(eventName) { },

  
  











  _hasRemovableSelection: function PC__hasRemovableSelection(aIsMoveCommand) {
    if (!this._view.hasSelection)
      return false;

    var nodes = this._view.getSelectionNodes();
    var root = this._view.getResultNode();

    for (var i = 0; i < nodes.length; ++i) {
      
      if (nodes[i] == root)
        return false;

      
      var nodeItemId = nodes[i].itemId;
      if (!aIsMoveCommand &&
           PlacesUtils.nodeIsFolder(nodes[i]) &&
           (nodeItemId == PlacesUtils.toolbarFolderId ||
            nodeItemId == PlacesUtils.unfiledBookmarksFolderId ||
            nodeItemId == PlacesUtils.bookmarksMenuFolderId))
        return false;

      
      
      
      
      
      
      
      
      
      var parent = nodes[i].parent || root;
      if (PlacesUtils.isReadonlyFolder(parent))
        return false;
    }
    return true;
  },

  


  _canInsert: function PC__canInsert() {
    return this._view.insertionPoint != null;
  },

  


  rootNodeIsSelected: function PC_rootNodeIsSelected() {
    if (this._view.hasSelection) {
      var nodes = this._view.getSelectionNodes();
      var root = this._view.getResultNode();
      for (var i = 0; i < nodes.length; ++i) {
        if (nodes[i] == root)
          return true;      
      }
    }
    return false;
  },

  







  _isClipboardDataPasteable: function PC__isClipboardDataPasteable() {
    
    

    var flavors = PlacesUtils.placesFlavors;
    var clipboard = PlacesUtils.clipboard;
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
    var nodes = [];
    var root = this._view.getResult().root;
    if (this._view.hasSelection)
      nodes = this._view.getSelectionNodes();
    else 
      nodes = [root];

    for (var i=0; i < nodes.length; i++) {
      var nodeData = {};
      var node = nodes[i];
      var nodeType = node.type;
      var uri = null;

      
      
      switch(nodeType) {
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY:
          nodeData["query"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_DYNAMIC_CONTAINER:
          nodeData["dynamiccontainer"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER:
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER_SHORTCUT:
          nodeData["folder"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_HOST:
          nodeData["host"] = true;
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
            var mss = PlacesUtils.microsummaries;
            if (mss.hasMicrosummary(node.itemId))
              nodeData["microsummary"] = true;
            else if (node.parent &&
                     PlacesUtils.nodeIsLivemarkContainer(node.parent))
              nodeData["livemarkChild"] = true;
          }
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_DAY:
          nodeData["day"] = true;
      }

      
      
      
      
      
      if (!PlacesUtils.nodeIsReadOnly(node) &&
          !PlacesUtils.isReadonlyFolder(node.parent || root))
        nodeData["mutable"] = true;

      
      if (uri) {
        var names = PlacesUtils.annotations.getPageAnnotationNames(uri, {});
        for (var j = 0; j < names.length; ++j)
          nodeData[names[j]] = true;
      }

      
      if (node.itemId != -1) {
        names = PlacesUtils.annotations
                           .getItemAnnotationNames(node.itemId, {});
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

    var forceHideRules = aMenuItem.getAttribute("forcehideselection").split("|");
    for (var i = 0; i < aMetaData.length; ++i) {
      for (var j=0; j < forceHideRules.length; ++j) {
        if (forceHideRules[j] in aMetaData[i])
          return false;
      }
    }

    if (aMenuItem.hasAttribute("selection")) {
      var showRules = aMenuItem.getAttribute("selection").split("|");
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

    var separator = null;
    var visibleItemsBeforeSep = false;
    var anyVisible = false;
    for (var i = 0; i < aPopup.childNodes.length; ++i) {
      var item = aPopup.childNodes[i];
      if (item.localName != "menuseparator") {
        item.hidden = !this._shouldShowMenuItem(item, metadata);
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
          PlacesUtils.getURLsForContainerNode(this._view.selectedNode)
                     .length == 0;
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

  







  openSelectedNodeWithEvent: function PC_openSelectedNodeWithEvent(aEvent) {
    this.openSelectedNodeIn(whereToOpenLink(aEvent));
  },

  




  openSelectedNodeIn: function PC_openSelectedNodeIn(aWhere) {
    var node = this._view.selectedURINode;
    if (node && PlacesUtils.checkURLSecurity(node)) {
      var isBookmark = PlacesUtils.nodeIsBookmark(node);

      if (isBookmark)
        PlacesUtils.markPageAsFollowedBookmark(node.uri);
      else
        PlacesUtils.markPageAsTyped(node.uri);

      
      
      if (aWhere == "current" && isBookmark) {
        if (PlacesUtils.annotations
                       .itemHasAnnotation(node.itemId, LOAD_IN_SIDEBAR_ANNO)) {
          var w = getTopWin();
          if (w) {
            w.openWebPanel(node.title, node.uri);
            return;
          }
        }
      }
      openUILinkIn(node.uri, aWhere);
    }
  },

  


  showBookmarkPropertiesForSelection: 
  function PC_showBookmarkPropertiesForSelection() {
    var node = this._view.selectedNode;
    if (!node)
      return;

    if (PlacesUtils.nodeIsFolder(node))
      PlacesUtils.showFolderProperties(node.itemId);
    else if (PlacesUtils.nodeIsBookmark(node))
      PlacesUtils.showBookmarkProperties(node.itemId);
  },

  



  _assertURINotString: function PC__assertURINotString(value) {
    NS_ASSERT((typeof(value) == "object") && !(value instanceof String), 
           "This method should be passed a URI as a nsIURI object, not as a string.");
  },

  


  reloadSelectedLivemark: function PC_reloadSelectedLivemark() {
    if (this._view.hasSingleSelection) {
      var selectedNode = this._view.selectedNode;
      if (PlacesUtils.nodeIsLivemarkContainer(selectedNode))
        PlacesUtils.livemarks.reloadLivemarkFolder(selectedNode.itemId);
    }
  },

  


  reloadSelectedMicrosummary: function PC_reloadSelectedMicrosummary() {
    var selectedNode = this._view.selectedNode;
    var mss = PlacesUtils.microsummaries;
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
          PlacesUtils.getString("tabs.openWarningTitle"),
          PlacesUtils.getFormattedString(messageKey, 
            [numTabsToOpen, brandShortName]),
          (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0)
          + (promptService.BUTTON_TITLE_CANCEL * promptService.BUTTON_POS_1),
          PlacesUtils.getString(openKey),
          null, null,
          PlacesUtils.getFormattedString("tabs.openWarningPromptMeBranded",
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
    if (this._view.hasSingleSelection && PlacesUtils.nodeIsContainer(node))
      PlacesUtils.openContainerNodeInTabs(this._view.selectedNode, aEvent);
    else
      PlacesUtils.openURINodesInTabs(this._view.getSelectionNodes(), aEvent);
  },

  





  newItem: function PC_newItem(aType) {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    this._view.saveSelection(this._view.SAVE_SELECTION_INSERT);

    var performed = false;
    if (aType == "bookmark")
      performed = PlacesUtils.showAddBookmarkUI(null, null, null, ip);
    else if (aType == "livemark")
      performed = PlacesUtils.showAddLivemarkUI(null, null, null, null, ip);
    else 
      performed = PlacesUtils.showAddFolderUI(null, ip);

    if (performed)
      this._view.restoreSelection();
  },


  



  newFolder: function PC_newFolder() {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;

    this._view.saveSelection(this._view.SAVE_SELECTION_INSERT);
    if (PlacesUtils.showAddFolderUI(null, ip))
      this._view.restoreSelection();
  },

  


  newSeparator: function PC_newSeparator() {
    var ip = this._view.insertionPoint;
    if (!ip)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    var txn = PlacesUtils.ptm.createSeparator(ip.itemId, ip.index);
    PlacesUtils.ptm.doTransaction(txn);
  },

  


  moveSelectedBookmarks: function PC_moveBookmarks() {
    window.openDialog("chrome://browser/content/places/moveBookmarks.xul",
                      "", "chrome, modal",
                      this._view.getSelectionNodes());
  },

  


  sortFolderByName: function PC_sortFolderByName() {
    var selectedNode = this._view.selectedNode;
    var txn = PlacesUtils.ptm.sortFolderByName(selectedNode.itemId,
                                               selectedNode.bookmarkIndex);
    PlacesUtils.ptm.doTransaction(txn);
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

  







  _removeRange: function PC__removeRange(range, transactions) {
    NS_ASSERT(transactions instanceof Array, "Must pass a transactions array");

    var removedFolders = [];

    for (var i = 0; i < range.length; ++i) {
      var node = range[i];
      if (this._shouldSkipNode(node, removedFolders))
        continue;

      if (PlacesUtils.nodeIsFolder(node))
        removedFolders.push(node);

      transactions.push(PlacesUtils.ptm.removeItem(node.itemId));
    }
  },

  




  _removeRowsFromBookmarks: function PC__removeRowsFromBookmarks(txnName) {
    var ranges = this._view.getRemovableSelectionRanges();
    var transactions = [];
    
    
    for (var i = ranges.length - 1; i >= 0 ; --i)
      this._removeRange(ranges[i], transactions);
    if (transactions.length > 0) {
      var txn = PlacesUtils.ptm.aggregateTransactions(txnName, transactions);
      PlacesUtils.ptm.doTransaction(txn);
    }
  },

  


  _removeRowsFromHistory: function PC__removeRowsFromHistory() {
    
    
    var nodes = this._view.getSelectionNodes();
    for (var i = 0; i < nodes.length; ++i) {
      var node = nodes[i];
      var bhist = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
      if (PlacesUtils.nodeIsHost(node))
        bhist.removePagesFromHost(node.title, true);
      else if (PlacesUtils.nodeIsURI(node))
        bhist.removePage(PlacesUtils._uri(node.uri));
    }
  },

  





  remove: function PC_remove(aTxnName) {
    NS_ASSERT(aTxnName !== undefined, "Must supply Transaction Name");

    var root = this._view.getResult().root;

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

  







  getTransferData: function PC_getTransferData(dragAction) {
    var result = this._view.getResult();
    var oldViewer = result.viewer;
    try {
      result.viewer = null;
      var nodes = null;
      if (dragAction == Ci.nsIDragService.DRAGDROP_ACTION_COPY)
        nodes = this._view.getCopyableSelection();
      else
        nodes = this._view.getDragableSelection();
      var dataSet = new TransferDataSet();
      for (var i = 0; i < nodes.length; ++i) {
        var node = nodes[i];

        var data = new TransferData();
        function addData(type, overrideURI) {
          data.addDataForFlavour(type, PlacesUtils._wrapString(
                                 PlacesUtils.wrapNode(node, type, overrideURI)));
        }

        function addURIData(overrideURI) {
          addData(PlacesUtils.TYPE_X_MOZ_URL, overrideURI);
          addData(PlacesUtils.TYPE_UNICODE, overrideURI);
          addData(PlacesUtils.TYPE_HTML, overrideURI);
        }

        
        
        addData(PlacesUtils.TYPE_X_MOZ_PLACE);
      
        var uri;
      
        
        if (PlacesUtils.nodeIsLivemarkContainer(node))
          uri = PlacesUtils.livemarks.getFeedURI(node.itemId).spec;
      
        addURIData(uri);
        dataSet.push(data);
      }
    }
    finally {
      if (oldViewer)
        result.viewer = oldViewer;
    }
    return dataSet;
  },

  


  copy: function PC_copy() {
    var result = this._view.getResult();
    var oldViewer = result.viewer;
    try {
      result.viewer = null;
      var nodes = this._view.getCopyableSelection();

      var xferable =  Cc["@mozilla.org/widget/transferable;1"].
                      createInstance(Ci.nsITransferable);
      var foundFolder = false, foundLink = false;
      var copiedFolders = [];
      var placeString = mozURLString = htmlString = unicodeString = "";
    
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
          return PlacesUtils.wrapNode(node, type, overrideURI) + placeSuffix;
        }

        
        placeString += generateChunk(PlacesUtils.TYPE_X_MOZ_PLACE);
      }

      function addData(type, data) {
        xferable.addDataFlavor(type);
        xferable.setTransferData(type, PlacesUtils._wrapString(data), data.length * 2);
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
        PlacesUtils.clipboard.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
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

    var clipboard = PlacesUtils.clipboard;

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
          
          
          if (ip.index > -1)
            index = ip.index + i;
          transactions.push(PlacesUtils.makeTransaction(items[i], type.value, 
                                                        ip.itemId, index,
                                                        true));
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
    var txn = PlacesUtils.ptm.aggregateTransactions("Paste", transactions);
    PlacesUtils.ptm.doTransaction(txn);
  }
};

function PlacesMenuDNDObserver(aView, aPopup) {
  this._view = aView;
  this._popup = aPopup;
  this._popup.addEventListener("draggesture", this, false);
  this._popup.addEventListener("dragover", this, false);
  this._popup.addEventListener("dragdrop", this, false);
  this._popup.addEventListener("dragexit", this, false);
}






PlacesMenuDNDObserver.prototype = {
  _view: null,
  _popup: null,

  
  
  
  _overFolder: {node: null, openTimer: null, hoverTime: 350, closeTimer: null},

  
  
  
  _closeMenuTimer: null,

  _setTimer: function TBV_DO_setTimer(time) {
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, time, timer.TYPE_ONE_SHOT);
    return timer;
  },

  
  notify: function TBV_DO_notify(timer) {
    
    if (timer == this._overFolder.openTimer) {
      this._overFolder.node.lastChild.setAttribute("autoopened", "true");
      this._overFolder.node.lastChild.showPopup(this._overFolder.node);
      this._overFolder.openTimer = null;
    }

    
    if (timer == this._overFolder.closeTimer) {
      
      
      var draggingOverChild =
        PlacesControllerDragHelper.draggingOverChildNode(this._overFolder.node);
      if (draggingOverChild)
        this._overFolder.node = null;
      this._clearOverFolder();

      
      
      
      if (!draggingOverChild)
        this._closeParentMenus();
    }

    
    if (timer == this._closeMenuTimer) {
      if (!PlacesControllerDragHelper.draggingOverChildNode(this._popup)) {
        this._popup.hidePopup();
        
        
        
        this._closeParentMenus();
      }
    }
  },

  
  
  
  _closeParentMenus: function TBV_DO_closeParentMenus() {
    var parent = this._popup.parentNode;
    while (parent) {
      if (parent.nodeName == "menupopup" && parent._resultNode) {
        if (PlacesControllerDragHelper.draggingOverChildNode(parent.parentNode))
          break;
        parent.hidePopup();
      }
      parent = parent.parentNode;
    }
  },

  
  
  
  _clearOverFolder: function TBV_DO_clearOverFolder() {
    if (this._overFolder.node && this._overFolder.node.lastChild) {
      if (!this._overFolder.node.lastChild.hasAttribute("dragover"))
        this._overFolder.node.lastChild.hidePopup();
      this._overFolder.node = null;
    }
    if (this._overFolder.openTimer) {
      this._overFolder.openTimer.cancel();
      this._overFolder.openTimer = null;
    }
    if (this._overFolder.closeTimer) {
      this._overFolder.closeTimer.cancel();
      this._overFolder.closeTimer = null;
    }
  },

  
  
  
  _getDropPoint: function TBV_DO_getDropPoint(event) {
    
    var resultNode = this._popup._resultNode;
    if (!PlacesUtils.nodeIsFolder(resultNode))
      return null;

    var dropPoint = { ip: null, beforeIndex: null, folderNode: null };
    
    
    var start = 0;
    var end = this._popup.childNodes.length;
    if (this._popup == this._view && this._view.localName == "menupopup") {
      
      start = this._view._startMarker + 1;
      if (this._view._endMarker != -1)
        end = this._view._endMarker;
    }

    var popupFirstChildY = this._popup.firstChild.boxObject.y;
    for (var i = start; i < end; i++) {
      var xulNode = this._popup.childNodes[i];
      var nodeY = xulNode.boxObject.y - popupFirstChildY;
      var nodeHeight = xulNode.boxObject.height;
      if (xulNode.node &&
          PlacesUtils.nodeIsFolder(xulNode.node) &&
          !PlacesUtils.nodeIsReadOnly(xulNode.node)) {
        
        
        
        if (event.layerY < nodeY + (nodeHeight * 0.25)) {
          
          dropPoint.ip = new InsertionPoint(resultNode.itemId, i - start,
                                            -1);
          dropPoint.beforeIndex = i;
          return dropPoint;
        }
        else if (event.layerY < nodeY + (nodeHeight * 0.75)) {
          
          dropPoint.ip = new InsertionPoint(xulNode.node.itemId, -1, 1);
          dropPoint.beforeIndex = i;
          dropPoint.folderNode = xulNode;
          return dropPoint;
        }
      } else {
        
        
        if (event.layerY < nodeY + (nodeHeight / 2)) {
          
          dropPoint.ip = new InsertionPoint(resultNode.itemId, i - start, -1);
          dropPoint.beforeIndex = i;
          return dropPoint;
        }
      }
    }
    
    dropPoint.ip = new InsertionPoint(resultNode.itemId, -1, 1);
    dropPoint.beforeIndex = -1;
    return dropPoint;
  },

  
  
  _clearStyles: function TBV_DO_clearStyles() {
    this._popup.removeAttribute("dragover");
    for (var i = 0; i < this._popup.childNodes.length; i++) {
      this._popup.childNodes[i].removeAttribute("dragover-top");
      this._popup.childNodes[i].removeAttribute("dragover-bottom");
      this._popup.childNodes[i].removeAttribute("dragover-into");
    }
  },

  onDragStart: function TBV_DO_onDragStart(event, xferData, dragAction) {
    this._view._selection = event.target.node;
    this._view._cachedInsertionPoint = undefined;
    if (event.ctrlKey)
      dragAction.action = Ci.nsIDragService.DRAGDROP_ACTION_COPY;
    xferData.data = this._view.controller.getTransferData(dragAction.action);
  },

  canDrop: function TBV_DO_canDrop(event, session) {
    return PlacesControllerDragHelper.canDrop(this._view, -1);
  },

  onDragOver: function TBV_DO_onDragOver(event, flavor, session) {
    PlacesControllerDragHelper.currentDropTarget = event.target;
    var dropPoint = this._getDropPoint(event);
    if (dropPoint == null)
      return;

    this._clearStyles();
    if (dropPoint.folderNode) {
      
      if (this._overFolder.node != dropPoint.folderNode) {
        this._clearOverFolder();
        this._overFolder.node = dropPoint.folderNode;
        this._overFolder.openTimer = this._setTimer(this._overFolder.hoverTime);
      }
      dropPoint.folderNode.setAttribute("dragover-into", "true");
    }
    else {
      
      
      if (dropPoint.beforeIndex == -1) {
        if (this._popup == this._view && this._view.localName == "menupopup" &&
            this._popup._endMarker != -1) {
          this._popup.childNodes[this._popup._endMarker]
                     .setAttribute("dragover-top", "true");
        }
        else
          this._popup.lastChild.setAttribute("dragover-bottom", "true");
      }
      else {
        this._popup.childNodes[dropPoint.beforeIndex]
            .setAttribute("dragover-top", "true");
      }

      
      this._clearOverFolder();
    }
    this._popup.setAttribute("dragover", "true");
  },

  onDrop: function TBV_DO_onDrop(event, dropData, session) {
    var dropPoint = this._getDropPoint(event);
    if (!dropPoint)
      return;

    PlacesControllerDragHelper.onDrop(dropPoint.ip);
  },

  onDragExit: function TBV_DO_onDragExit(event, session) {
    PlacesControllerDragHelper.currentDropTarget = null;
    this._clearStyles();
    
    if (this._overFolder.node)
      this._overFolder.closeTimer = this._setTimer(this._overFolder.hoverTime);
    
    
    
    if (this._popup.hasAttribute("autoopened"))
      this._closeMenuTimer = this._setTimer(this._overFolder.hoverTime);
  },

  getSupportedFlavours: function TBV_DO_getSupportedFlavours() {
    var flavorSet = new FlavourSet();
    var types = PlacesUtils.GENERIC_VIEW_DROP_TYPES;
    for (var i = 0; i < types; ++i)
      flavorSet.appendFlavour(types[i]);
    return flavorSet;
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
    case "draggesture":
      if (aEvent.target.localName != "menu" && aEvent.target.node) {
        
        nsDragAndDrop.startDrag(aEvent, this);
      }
      break;
    case "dragover":
      nsDragAndDrop.dragOver(aEvent, this);
      break;
    case "dragdrop":
      nsDragAndDrop.drop(aEvent, this);
      break;
    case "dragexit":
      nsDragAndDrop.dragExit(aEvent, this);
      break;
    }
  }
}







var PlacesControllerDragHelper = {

  








  draggingOverChildNode: function PCDH_draggingOverChildNode(node) {
    var currentNode = this.currentDropTarget;
    while (currentNode) {
      if (currentNode == node)
        return true;
      currentNode = currentNode.parentNode;
    }
    return false;
  },

  


  currentDropTarget: null,

  


  getSession: function VO__getSession() {
    var dragService = Cc["@mozilla.org/widget/dragservice;1"].
                      getService(Ci.nsIDragService);
    return dragService.getCurrentSession();
  },

  









  canDrop: function PCDH_canDrop(view, orientation) {
    var root = view.result.root;
    if (PlacesUtils.nodeIsReadOnly(root) || 
        !PlacesUtils.nodeIsFolder(root))
      return false;

    var session = this.getSession();
    if (session) {
      var types = PlacesUtils.GENERIC_VIEW_DROP_TYPES;
      for (var i = 0; i < types.length; ++i) {
        if (session.isDataFlavorSupported(types[i]))
          return true;
      }
    }
    return false;
  },

  







  _initTransferable: function PCDH__initTransferable(session) {
    var xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    var types = PlacesUtils.GENERIC_VIEW_DROP_TYPES;
    for (var i = 0; i < types.length; ++i) {
      if (session.isDataFlavorSupported(types[i]))
        xferable.addDataFlavor(types[i]);
    }
    return xferable;
  },

  




  onDrop: function PCDH_onDrop(insertionPoint) {
    var session = this.getSession();
    var copy = session.dragAction & Ci.nsIDragService.DRAGDROP_ACTION_COPY;
    var transactions = [];
    var xferable = this._initTransferable(session);
    var dropCount = session.numDropItems;

    var movedCount = 0;

    for (var i = 0; i < dropCount; ++i) {
      session.getData(xferable, i);

      var data = { }, flavor = { };
      xferable.getAnyTransferData(flavor, data, { });
      data.value.QueryInterface(Ci.nsISupportsString);

      
      var unwrapped = PlacesUtils.unwrapNodes(data.value.data, 
                                              flavor.value)[0];
      var index = insertionPoint.index;

      
      
      
      if ((index != -1) && ((index < unwrapped.index) ||
                           (unwrapped.folder && (index < unwrapped.folder.index)))) {
        index = index + movedCount;
        movedCount++;
      }

      transactions.push(PlacesUtils.makeTransaction(unwrapped,
                        flavor.value, insertionPoint.itemId,
                        index, copy));
    }

    var txn = PlacesUtils.ptm.aggregateTransactions("DropItems", transactions);
    PlacesUtils.ptm.doTransaction(txn);
  }
};

function goUpdatePlacesCommands() {
  var placesController;
  try {
    
    placesController = top.document.commandDispatcher
                          .getControllerForCommand("placesCmd_open");
  }
  catch(ex) { return; }

  function updatePlacesCommand(aCommand) {
    var enabled = false;
    if (placesController)
      enabled = placesController.isCommandEnabled(aCommand);
    goSetCommandEnabled(aCommand, enabled);
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
}
