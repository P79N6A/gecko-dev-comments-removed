






































const NHRVO = Ci.nsINavHistoryResultViewObserver;


const ORGANIZER_ROOT_HISTORY_UNSORTED = "place:type=1";
const ORGANIZER_ROOT_BOOKMARKS = "place:folder=2&group=3&excludeItems=1&queryType=1";
const ORGANIZER_SUBSCRIPTIONS_QUERY = "place:annotation=livemark%2FfeedURI";


const RELOAD_ACTION_NOTHING = 0;

const RELOAD_ACTION_INSERT = 1;

const RELOAD_ACTION_REMOVE = 2;


const RELOAD_ACTION_MOVE = 3;

#ifdef XP_MACOSX


const NEWLINE= "\n";
#else

const NEWLINE = "\r\n";
#endif















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
      return PlacesUtils.tm.numberOfUndoItems > 0;
    case "cmd_redo":
      return PlacesUtils.tm.numberOfRedoItems > 0;
    case "cmd_cut":
    case "cmd_delete":
      return this._hasRemovableSelection(false);
    case "placesCmd_moveBookmarks":
      return this._hasRemovableSelection(true);
    case "cmd_copy":
      return this._view.hasSelection;
    case "cmd_paste":
      return this._canInsert() && this._canPaste();
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
    case "placesCmd_open:tabs":
      
      
      
      var node = this._view.selectedNode;
      if (!node)
        return false;

      if (this._view.hasSingleSelection && PlacesUtils.nodeIsFolder(node)) {
        var contents = PlacesUtils.getFolderContents(node.itemId, false, false).root;
        for (var i = 0; i < contents.childCount; ++i) {
          var child = contents.getChild(i);
          if (PlacesUtils.nodeIsURI(child))
            return true;
        }
      }
      else {
        var oneLinkIsSelected = false;
        var nodes = this._view.getSelectionNodes();
        for (var i = 0; i < nodes.length; ++i) {
          if (PlacesUtils.nodeIsURI(nodes[i])) {
            if (oneLinkIsSelected)
              return true;
            oneLinkIsSelected = true;
          }
        }
      }
      return false;
#ifdef MOZ_PLACES_BOOKMARKS
    case "placesCmd_new:folder":
    case "placesCmd_new:livemark":
      return this._canInsert() &&
             this._view.peerDropTypes
                 .indexOf(PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER) != -1;
    case "placesCmd_new:bookmark":
      return this._canInsert() &&
             this._view.peerDropTypes.indexOf(PlacesUtils.TYPE_X_MOZ_URL) != -1;
    case "placesCmd_new:separator":
      return this._canInsert() &&
             this._view.peerDropTypes
                 .indexOf(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR) != -1 &&
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

#ifdef EXTENDED_LIVEBOOKMARKS_UI
        
        if (selectedNode.uri.indexOf("livemark%2F") != -1)
          return true;

        
        
        if (PlacesUtils.nodeIsURI() &&
            PlacesUtils.nodeIsLivemarkItem(selectedNode))
          return true;
#endif
      }
      return false;
    case "placesCmd_sortBy:name":
      var selectedNode = this._view.selectedNode;
      return selectedNode &&
             PlacesUtils.nodeIsFolder(selectedNode) &&
             !PlacesUtils.nodeIsReadOnly(selectedNode) &&
             this._view.getResult().sortingMode ==
                 Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
    case "placesCmd_setAsBookmarksToolbarFolder":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;
        if (PlacesUtils.nodeIsFolder(selectedNode) &&
            selectedNode.itemId != PlacesUtils.toolbarFolderId) {
          return true;
        }
      }
      return false;
#endif
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
      PlacesUtils.tm.undoTransaction();
      break;
    case "cmd_redo":
      PlacesUtils.tm.redoTransaction();
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
    case "placesCmd_open:tabs":
      this.openLinksInTabs();
      break;
#ifdef MOZ_PLACES_BOOKMARKS
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
      this.reloadSelectedLivemarks();
      break;
    case "placesCmd_reloadMicrosummary":
      this.reloadSelectedMicrosummary();
      break;
    case "placesCmd_sortBy:name":
      this.sortFolderByName();
      break;
    case "placesCmd_setAsBookmarksToolbarFolder":
      this.setBookmarksToolbarFolder();
      break;
#endif
    }
  },

  onEvent: function PC_onEvent(eventName) { },

  
  











  _hasRemovableSelection: function PC__hasRemovableSelection(aIsMoveCommand) {
    if (!this._view.hasSelection)
      return false;

    var nodes = this._view.getSelectionNodes();
    var root = this._view.getResultNode();

    var btFolderId = PlacesUtils.toolbarFolderId;
    for (var i = 0; i < nodes.length; ++i) {
      
      if (nodes[i] == root)
        return false;

      
      if (!aIsMoveCommand &&
          PlacesUtils.nodeIsFolder(nodes[i]) && nodes[i].itemId == btFolderId)
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

  






  _canPaste: function PC__canPaste() {
    var types = this._view.peerDropTypes;
    var flavors = 
        Cc["@mozilla.org/supports-array;1"].
        createInstance(Ci.nsISupportsArray);
    for (var i = 0; i < types.length; ++i) {
      var cstring = 
          Cc["@mozilla.org/supports-cstring;1"].
          createInstance(Ci.nsISupportsCString);
      cstring.data = types[i];
      flavors.AppendElement(cstring);
    }

    var clipboard = 
        Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
    var hasClipboardData = clipboard.hasDataMatchingFlavors(flavors, 
                                            Ci.nsIClipboard.kGlobalClipboard);
    if (!hasClipboardData)
      return false;

    
    
    
    

    var xferable = 
        Cc["@mozilla.org/widget/transferable;1"].
        createInstance(Ci.nsITransferable);

    for (var j = 0; j < types.length; ++j) {
      xferable.addDataFlavor(types[j]);
    }
    clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);

    try {
      
      var data = { }, type = { };
      xferable.getAnyTransferData(type, data, { });
      data = data.value.QueryInterface(Ci.nsISupportsString).data;
      if (this._view.peerDropTypes.indexOf(type.value) == -1)
        return false;

      
      var unwrappedNodes = PlacesUtils.unwrapNodes(data, type.value);
      return this._view.insertionPoint != null;
    }
    catch (e) {
      
      
      
      return false;
    }
    return false;
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
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_REMOTE_CONTAINER:
          nodeData["remotecontainer"] = true;
          break;
        case Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER:
          nodeData["folder"] = true;
          uri = PlacesUtils.bookmarks.getFolderURI(node.itemId);

          
          if (asContainer(node).remoteContainerType != "")
            nodeData["remotecontainer"] = true;
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

        
        if ("bookmark" in nodeData || "folder" in nodeData) {
          names = PlacesUtils.annotations
                             .getItemAnnotationNames(node.itemId, {});
          for (j = 0; j < names.length; ++j)
            nodeData[names[j]] = true;
        }
      }
#ifdef EXTENDED_LIVEBOOKMARKS_UI
      else if (nodeType == Ci.nsINavHistoryResultNode.RESULT_TYPE_QUERY) {
        
        
        
        
        
        
        
        uri = PlacesUtils._uri(nodes[i].uri);
        if (uri.spec == ORGANIZER_SUBSCRIPTIONS_QUERY)
          nodeData["allLivemarks"] = true;
      }
#endif
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
       
       
      if (aWhere == "current" && PlacesUtils.nodeIsBookmark(node)) {
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

  





  reloadSelectedLivemarks: function PC_reloadSelectedLivemarks() {
    var selectedNode = this._view.selectedNode;
    if (this._view.hasSingleSelection) {
#ifdef EXTENDED_LIVEBOOKMARKS_UI
      if (selectedNode.uri.indexOf("livemark%2F") != -1) {
        PlacesUtils.livemarks.reloadAllLivemarks();
        return;
      }
#endif
      var folder = null;
      if (PlacesUtils.nodeIsLivemarkContainer(selectedNode)) {
        folder = selectedNode;
      }
#ifdef EXTENDED_LIVEBOOKMARKS_UI
      else if (PlacesUtils.nodeIsURI()) {
        if (PlacesUtils.nodeIsLivemarkItem(selectedNode))
          folder = selectedNode.parent;
      }
#endif
      if (folder)
        PlacesUtils.livemarks.reloadLivemarkFolder(folder.itemId);
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

  







  openLinksInTabs: function PC_openLinksInTabs() {
    var node = this._view.selectedNode;
    if (this._view.hasSingleSelection && PlacesUtils.nodeIsFolder(node)) {
      
      var doReplace = getBoolPref("browser.tabs.loadFolderAndReplace");
      var loadInBackground = getBoolPref("browser.tabs.loadBookmarksInBackground");
      

      
      var browserWindow = getTopWin();
      var browser = browserWindow.getBrowser();
      var tabPanels = browser.browsers;
      var tabCount = tabPanels.length;
      var firstIndex;
      
      
      if (doReplace)
        firstIndex = 0;
      
      else {
        for (firstIndex = tabCount - 1; firstIndex >= 0; --firstIndex) {
          var br = browser.browsers[firstIndex];
          if (br.currentURI.spec != "about:blank" ||
              br.webProgress.isLoadingDocument)
            break;
        }
        ++firstIndex;
      }

      
      var index = firstIndex;
      var urlsToOpen = [];
      var contents = PlacesUtils.getFolderContents(node.itemId, false, false).root;
      for (var i = 0; i < contents.childCount; ++i) {
        var child = contents.getChild(i);
        if (PlacesUtils.nodeIsURI(child))
          urlsToOpen.push(child.uri);
      }

      if (!this._confirmOpenTabs(urlsToOpen.length))
        return;

      for (var i = 0; i < urlsToOpen.length; ++i) {
        if (index < tabCount)
          tabPanels[index].loadURI(urlsToOpen[i]);
        
        else
          browser.addTab(urlsToOpen[i]);
        ++index;
      }

      
      if (index == firstIndex)
        return;

      
      if (!loadInBackground || doReplace) {
        
        
        
        function selectNewForegroundTab(browser, tab) {
          browser.selectedTab = tab;
        }
        var tabs = browser.mTabContainer.childNodes;
        setTimeout(selectNewForegroundTab, 0, browser, tabs[firstIndex]);
      }

      
      
      for (var i = tabCount - 1; i >= index; --i)
        browser.removeTab(tabs[i]);

      
      browserWindow.content.focus();
    }
    else {
      var urlsToOpen = [];
      var nodes = this._view.getSelectionNodes();

      for (var i = 0; i < nodes.length; ++i) {
        if (PlacesUtils.nodeIsURI(nodes[i]))
          urlsToOpen.push(nodes[i].uri);
      }

      if (!this._confirmOpenTabs(urlsToOpen.length))
        return;

      for (var i = 0; i < urlsToOpen.length; ++i) {
        getTopWin().openNewTabWith(urlsToOpen[i], null, null);
      }
    }
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
    PlacesUtils.ptm.commitTransaction(txn);
  },

  


  moveSelectedBookmarks: function PC_moveBookmarks() {
    window.openDialog("chrome://browser/content/places/moveBookmarks.xul",
                      "", "chrome, modal",
                      this._view.getSelectionNodes(), PlacesUtils.tm);
  },

  


  sortFolderByName: function PC_sortFolderByName() {
    var selectedNode = this._view.selectedNode;
    var txn = PlacesUtils.ptm.sortFolderByName(selectedNode.itemId,
                                               selectedNode.bookmarkIndex);
    PlacesUtils.ptm.commitTransaction(txn);
  },

  


  setBookmarksToolbarFolder: function PC_setBookmarksToolbarFolder() {
    if (!this._view.hasSingleSelection)
      return;

    var selectedNode = this._view.selectedNode;
    var txn = PlacesUtils.ptm.setBookmarksToolbar(selectedNode.itemId);
    PlacesUtils.ptm.commitTransaction(txn);
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
      PlacesUtils.ptm.commitTransaction(txn);
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
    this._view.saveSelection(this._view.SAVE_SELECTION_REMOVE);

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
      
    this._view.restoreSelection();
  },

  







  getTransferData: function PC_getTransferData(dragAction) {
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
        data.addDataForFlavour(type, PlacesUtils._wrapString(PlacesUtils.wrapNode(node, type, overrideURI)));
      }

      function addURIData(overrideURI) {
        addData(PlacesUtils.TYPE_X_MOZ_URL, overrideURI);
        addData(PlacesUtils.TYPE_UNICODE, overrideURI);
        addData(PlacesUtils.TYPE_HTML, overrideURI);
      }

      if (PlacesUtils.nodeIsFolder(node) || PlacesUtils.nodeIsQuery(node)) {
        
        
        

        addData(PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);

        
        if (PlacesUtils.nodeIsLivemarkContainer(node)) {
          var uri = PlacesUtils.livemarks.getFeedURI(node.itemId);
          addURIData(uri.spec);
        }

      }
      else if (PlacesUtils.nodeIsSeparator(node)) {
        addData(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR);
      }
      else {
        
        
        addData(PlacesUtils.TYPE_X_MOZ_PLACE);
        addURIData();
      }
      dataSet.push(data);
    }
    return dataSet;
  },

  


  copy: function PC_copy() {
    var nodes = this._view.getCopyableSelection();

    var xferable = 
        Cc["@mozilla.org/widget/transferable;1"].
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
        var suffix = i < (nodes.length - 1) ? "\n" : "";
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
      var clipboard = 
          Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
      clipboard.setData(xferable, null, Ci.nsIClipboard.kGlobalClipboard);
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

    var clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                    getService(Ci.nsIClipboard);

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
    PlacesUtils.ptm.commitTransaction(txn);
  }
};







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
    var parent = view.getResult().root;
    if (PlacesUtils.nodeIsReadOnly(parent) || 
        !PlacesUtils.nodeIsFolder(parent))
      return false;

    var session = this.getSession();
    if (session) {
      if (orientation != NHRVO.DROP_ON)
        var types = view.peerDropTypes;
      else
        types = view.childDropTypes;
      for (var i = 0; i < types.length; ++i) {
        if (session.isDataFlavorSupported(types[i]))
          return true;
      }
    }
    return false;
  },

  












  _initTransferable: function PCDH__initTransferable(session, view, orientation) {
    var xferable = Cc["@mozilla.org/widget/transferable;1"].
                   createInstance(Ci.nsITransferable);
    if (orientation != NHRVO.DROP_ON) 
      var types = view.peerDropTypes;
    else
      types = view.childDropTypes;    
    for (var i = 0; i < types.length; ++i) {
      if (session.isDataFlavorSupported(types[i]));
        xferable.addDataFlavor(types[i]);
    }
    return xferable;
  },

  








  onDrop: function PCDH_onDrop(sourceView, targetView, insertionPoint) {
    var session = this.getSession();
    var copy = session.dragAction & Ci.nsIDragService.DRAGDROP_ACTION_COPY;
    var transactions = [];
    var xferable = this._initTransferable(session, targetView, 
                                          insertionPoint.orientation);
    var dropCount = session.numDropItems;
    for (var i = dropCount - 1; i >= 0; --i) {
      session.getData(xferable, i);
    
      var data = { }, flavor = { };
      xferable.getAnyTransferData(flavor, data, { });
      data.value.QueryInterface(Ci.nsISupportsString);
      
      
      var unwrapped = PlacesUtils.unwrapNodes(data.value.data, 
                                              flavor.value)[0];
      transactions.push(PlacesUtils.makeTransaction(unwrapped, 
                        flavor.value, insertionPoint.itemId, 
                        insertionPoint.index, copy));
    }

    var txn = PlacesUtils.ptm.aggregateTransactions("DropItems", transactions);
    PlacesUtils.ptm.commitTransaction(txn);
  }
};

function goUpdatePlacesCommands() {
  goUpdateCommand("placesCmd_open");
  goUpdateCommand("placesCmd_open:window");
  goUpdateCommand("placesCmd_open:tab");
  goUpdateCommand("placesCmd_open:tabs");
#ifdef MOZ_PLACES_BOOKMARKS
  goUpdateCommand("placesCmd_new:folder");
  goUpdateCommand("placesCmd_new:bookmark");
  goUpdateCommand("placesCmd_new:livemark");
  goUpdateCommand("placesCmd_new:separator");
  goUpdateCommand("placesCmd_show:info");
  goUpdateCommand("placesCmd_moveBookmarks");
  goUpdateCommand("placesCmd_setAsBookmarksToolbarFolder");
  goUpdateCommand("placesCmd_reload");
  goUpdateCommand("placesCmd_reloadMicrosummary");
  goUpdateCommand("placesCmd_sortBy:name");
#endif
}
