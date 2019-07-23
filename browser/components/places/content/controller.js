






































const NHRVO = Ci.nsINavHistoryResultViewObserver;





const ORGANIZER_ROOT_HISTORY_UNSORTED = "place:beginTime=-2592000000000&beginTimeRef=1&endTime=7200000000&endTimeRef=2&type=1"
const ORGANIZER_ROOT_HISTORY = ORGANIZER_ROOT_HISTORY_UNSORTED + "&sort=" + Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING;
const ORGANIZER_ROOT_BOOKMARKS = "place:folder=2&group=3&excludeItems=1";
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















function InsertionPoint(aFolderId, aIndex, aOrientation) {
  this.folderId = aFolderId;
  this.index = aIndex;
  this.orientation = aOrientation;
}
InsertionPoint.prototype.toString = function IP_toString() {
  return "[object InsertionPoint(folder:" + this.folderId + ",index:" + this.index + ",orientation:" + this.orientation + ")]";
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
    case "placesCmd_moveBookmarks":
      return this._hasRemovableSelection();
    case "cmd_copy":
      return this._view.hasSelection;
    case "cmd_paste":
      return this._canInsert() && 
             this._hasClipboardData() && this._canPaste();
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
        var contents = PlacesUtils.getFolderContents(asFolder(node).folderId,
                                                     false, false);
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
                 .indexOf(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR) != -1;
    case "placesCmd_show:info":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;
        if (PlacesUtils.nodeIsFolder(selectedNode) ||
            (PlacesUtils.nodeIsBookmark(selectedNode) &&
            !PlacesUtils.nodeIsLivemarkItem(selectedNode)))
          return true;
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
    case "placesCmd_setAsBookmarksToolbarFolder":
      if (this._view.hasSingleSelection) {
        var selectedNode = this._view.selectedNode;
        if (PlacesUtils.nodeIsFolder(selectedNode) &&
            selectedNode.folderId != PlacesUtils.bookmarks.toolbarFolder) {
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
    case "placesCmd_setAsBookmarksToolbarFolder":
      this.setBookmarksToolbarFolder();
      break;
#endif
    }
  },

  onEvent: function PC_onEvent(eventName) { },

  
  








  _hasRemovableSelection: function PC__hasRemovableSelection() {
    if (!this._view.hasSelection)
      return false;

    var nodes = this._view.getSelectionNodes();
    var root = this._view.getResultNode();

    var btFolderId = PlacesUtils.toolbarFolderId;
    for (var i = 0; i < nodes.length; ++i) {
      
      if (nodes[i] == root)
        return false;

      
      if (PlacesUtils.nodeIsFolder(nodes[i]) &&
          asFolder(nodes[i]).folderId == btFolderId)
        return false;

      
      
      
      
      
      
      
      
      
      var parent = nodes[i].parent || root;
      if (PlacesUtils.isReadonlyFolder(parent))
        return false;
    }
    return true;
  },

  





  _hasClipboardData: function PC__hasClipboardData() {
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
    return clipboard.hasDataMatchingFlavors(flavors, 
                                            Ci.nsIClipboard.kGlobalClipboard);
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

#ifdef BROKEN_SORT_CODE
  











  _updateSortCommands: 
  function PC__updateSortCommands(inSysArea, hasSingleSelection, selectedNode, 
                                  canInsert) {
    
    
    var result = this._view.getResult();
    var viewIsFolder = result ? PlacesUtils.nodeIsFolder(result.root) : false;

    
    
    
    
    var sortingChildren = false;
    var name = result.root.title;
    var sortFolder = result.root;
    if (selectedNode && selectedNode.parent) {
      name = selectedNode.parent.title;
      sortFolder = selectedNode.parent;
    }
    if (hasSingleSelection && PlacesUtils.nodeIsFolder(selectedNode)) {
      name = selectedNode.title;
      sortFolder = selectedNode;
      sortingChildren = true;
    }

    
    
    
    
    
    var enoughChildrenToSort = false;
    if (PlacesUtils.nodeIsFolder(sortFolder)) {
      var folder = asFolder(sortFolder);
      var contents = this.getFolderContents(folder.folderId, false, false);
      enoughChildrenToSort = contents.childCount > 1;
    }
    var metadata = this._buildSelectionMetadata();
    this._setEnabled("placesCmd_sortby:name", 
      (sortingChildren || !inSysArea) && canInsert && viewIsFolder && 
      !("mixed" in metadata) && enoughChildrenToSort);

    var command = document.getElementById("placesCmd_sortby:name");
    
    if (name) {
      command.setAttribute("label", 
        PlacesUtils.getFormattedString("sortByName", [name]));
    }
    else
      command.setAttribute("label", PlacesUtils.getString("sortByNameGeneric"));
  },
#endif

  








  _canPaste: function PC__canPaste() {
    var xferable = 
        Cc["@mozilla.org/widget/transferable;1"].
        createInstance(Ci.nsITransferable);
    xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);
    xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR);
    xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_PLACE);
    xferable.addDataFlavor(PlacesUtils.TYPE_X_MOZ_URL);

    var clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                    getService(Ci.nsIClipboard);
    clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);

    try {
      
      var data = { }, type = { };
      xferable.getAnyTransferData(type, data, { });
      data = data.value.QueryInterface(Ci.nsISupportsString).data;
      if (this._view.peerDropTypes.indexOf(type.value) == -1)
        return false;

      
      var nodes = PlacesUtils.unwrapNodes(data, type.value);

      var ip = this._view.insertionPoint;
      return ip != null;
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
          uri = PlacesUtils.bookmarks.getFolderURI(asFolder(node).folderId);

          
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
          if (PlacesUtils.nodeIsBookmark(node))
            nodeData["bookmark"] = true;
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
          nodeData[names[i]] = true;

        
        if ("bookmark" in nodeData) {
          var placeURI = PlacesUtils.bookmarks.getItemURI(node.bookmarkId);
          names = PlacesUtils.annotations.getPageAnnotationNames(placeURI, {});
          for (j = 0; j < names.length; ++j)
            nodeData[names[i]] = true;
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
        var placeURI = PlacesUtils.bookmarks.getItemURI(node.bookmarkId);
        if (PlacesUtils.annotations
                       .hasAnnotation(placeURI, LOAD_IN_SIDEBAR_ANNO)) {
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
      PlacesUtils.showFolderProperties(asFolder(node).folderId);
    else if (PlacesUtils.nodeIsBookmark(node))
      PlacesUtils.showBookmarkProperties(node.bookmarkId);
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
        folder = asFolder(selectedNode);
      }
#ifdef EXTENDED_LIVEBOOKMARKS_UI
      else if (PlacesUtils.nodeIsURI()) {
        if (PlacesUtils.nodeIsLivemarkItem(selectedNode))
          folder = asFolder(selectedNode.parent);
      }
#endif
      if (folder)
        PlacesUtils.livemarks.reloadLivemarkFolder(folder.folderId);
    }
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
      var contents = PlacesUtils.getFolderContents(asFolder(node).folderId,
                                                   false, false);
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
    var txn = new PlacesCreateSeparatorTransaction(ip.folderId, ip.index);
    PlacesUtils.tm.doTransaction(txn);
  },

  


  moveSelectedBookmarks: function PC_moveBookmarks() {
    window.openDialog("chrome://browser/content/places/moveBookmarks.xul",
                      "", "chrome, modal",
                      this._view.getSelectionNodes(), PlacesUtils.tm);
  },

  


  setBookmarksToolbarFolder: function PC_setBookmarksToolbarFolder() {
    if (!this._view.hasSingleSelection)
      return false;
    var selectedNode = this._view.selectedNode;
    var txn = new PlacesSetBookmarksToolbarTransaction(selectedNode.folderId);
    PlacesUtils.tm.doTransaction(txn);
  },


  







  _removeRange: function PC__removeRange(range, transactions) {
    NS_ASSERT(transactions instanceof Array, "Must pass a transactions array");
    var index = PlacesUtils.getIndexOfNode(range[0]);

    var removedFolders = [];

    







    function isContainedBy(node, parent) {
      var cursor = node.parent;
      while (cursor) {
        if (cursor == parent)
          return true;
        cursor = cursor.parent;
      }
      return false;
    }

    







    function shouldSkipNode(node) {
      for (var j = 0; j < removedFolders.length; ++j) {
        if (isContainedBy(node, removedFolders[j]))
          return true;
      }
      return false;          
    }

    for (var i = 0; i < range.length; ++i) {
      var node = range[i];
      if (shouldSkipNode(node))
        continue;

      if (PlacesUtils.nodeIsFolder(node)) {
        
        var folder = asFolder(node);
        removedFolders.push(folder);
        transactions.push(new PlacesRemoveFolderTransaction(folder.folderId));
      }
      else if (PlacesUtils.nodeIsSeparator(node)) {
        
        transactions.push(new PlacesRemoveSeparatorTransaction(
          asFolder(node.parent).folderId, index));
      }
      else if (PlacesUtils.nodeIsFolder(node.parent)) {
        
        transactions.push(new PlacesRemoveItemTransaction(node.bookmarkId,
          PlacesUtils._uri(node.uri), asFolder(node.parent).folderId, index));
      }
    }
  },

  




  _removeRowsFromBookmarks: function PC__removeRowsFromBookmarks(txnName) {
    var ranges = this._view.getRemovableSelectionRanges();
    var transactions = [];
    for (var i = ranges.length - 1; i >= 0 ; --i)
      this._removeRange(ranges[i], transactions);
    if (transactions.length > 0) {
      var txn = new PlacesAggregateTransaction(txnName, transactions);
      PlacesUtils.tm.doTransaction(txn);
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

    
    
    var type = this._view.getResult().root.type; 
    if (PlacesUtils.nodeIsFolder(this._view.getResult().root))
      this._removeRowsFromBookmarks(aTxnName);
    else
      this._removeRowsFromHistory();

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
          var uri = PlacesUtils.livemarks.getFeedURI(asFolder(node).folderId);
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
    var pcString = psString = placeString = mozURLString = htmlString = unicodeString = "";
    for (var i = 0; i < nodes.length; ++i) {
      var node = nodes[i];
      function generateChunk(type, overrideURI) {
        var suffix = i < (nodes.length - 1) ? NEWLINE : "";
        return PlacesUtils.wrapNode(node, type, overrideURI) + suffix;
      }

      function generateURIChunks(overrideURI) {
        mozURLString += generateChunk(PlacesUtils.TYPE_X_MOZ_URL, overrideURI);
        htmlString += generateChunk(PlacesUtils.TYPE_HTML, overrideURI);
        unicodeString += generateChunk(PlacesUtils.TYPE_UNICODE, overrideURI);
      }

      if (PlacesUtils.nodeIsFolder(node) || PlacesUtils.nodeIsQuery(node)) {
        pcString += generateChunk(PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);

        
        if (PlacesUtils.nodeIsLivemarkContainer(node)) {
          var uri = PlacesUtils.livemarks.getFeedURI(asFolder(node).folderId);
          generateURIChunks(uri.spec);
        }
      }
      else if (PlacesUtils.nodeIsSeparator(node))
        psString += generateChunk(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR);
      else {
        placeString += generateChunk(PlacesUtils.TYPE_X_MOZ_PLACE);
        generateURIChunks();
      }
    }

    function addData(type, data) {
      xferable.addDataFlavor(type);
      xferable.setTransferData(type, PlacesUtils._wrapString(data), data.length * 2);
    }
    
    
    if (pcString)
      addData(PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER, pcString);
    if (psString)
      addData(PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR, psString);
    if (placeString)
      addData(PlacesUtils.TYPE_X_MOZ_PLACE, placeString);
    if (mozURLString)
      addData(PlacesUtils.TYPE_X_MOZ_URL, mozURLString);
    if (unicodeString)
      addData(PlacesUtils.TYPE_UNICODE, unicodeString);
    if (htmlString)
      addData(PlacesUtils.TYPE_HTML, htmlString);

    if (pcString || psString || placeString || unicodeString || htmlString || 
        mozURLString) {
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
        for (var i = 0; i < items.length; ++i) {
          transactions.push(PlacesUtils.makeTransaction(items[i], type.value, 
                                                        ip.folderId, ip.index, true));
        }
        return transactions;
      }
      catch (e) {
        
        
        
        
        
      }
      return [];
    }

    
    
    var transactions = 
        [].concat(getTransactions([PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER]),
                  getTransactions([PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR]),
                  getTransactions([PlacesUtils.TYPE_X_MOZ_PLACE,
                                   PlacesUtils.TYPE_X_MOZ_URL, 
                                   PlacesUtils.TYPE_UNICODE]));
    var txn = new PlacesAggregateTransaction("Paste", transactions);
    PlacesUtils.tm.doTransaction(txn);
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
                        flavor.value, insertionPoint.folderId, 
                        insertionPoint.index, copy));
    }

    var txn = new PlacesAggregateTransaction("DropItems", transactions);
    PlacesUtils.tm.doTransaction(txn);
  }
};




function PlacesBaseTransaction() {
}
PlacesBaseTransaction.prototype = {
  utils: PlacesUtils,

  bookmarks: Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
             getService(Ci.nsINavBookmarksService),
  _livemarks: null,
  get livemarks() {
    if (!this._livemarks) {
      this._livemarks =
        Cc["@mozilla.org/browser/livemark-service;2"].
        getService(Ci.nsILivemarkService);
    }
    return this._livemarks;
  },

  
  
  MIN_TRANSACTIONS_FOR_BATCH: 5,

  LOG: LOG,
  redoTransaction: function PIT_redoTransaction() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  get isTransient() {
    return false;
  },

  merge: function PIT_merge(transaction) {
    return false;
  }
};




function PlacesAggregateTransaction(name, transactions) {
  this._transactions = transactions;
  this._name = name;
  this.container = -1;
  this.redoTransaction = this.doTransaction;
}
PlacesAggregateTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,
  
  doTransaction: function() {
    this.LOG("== " + this._name + " (Aggregate) ==============");
    if (this._transactions.length >= this.MIN_TRANSACTIONS_FOR_BATCH)
      this.bookmarks.beginUpdateBatch();
    for (var i = 0; i < this._transactions.length; ++i) {
      var txn = this._transactions[i];
      if (this.container > -1) 
        txn.container = this.container;
      txn.doTransaction();
    }
    if (this._transactions.length >= this.MIN_TRANSACTIONS_FOR_BATCH)
      this.bookmarks.endUpdateBatch();
    this.LOG("== " + this._name + " (Aggregate Ends) =========");
  },
  
  undoTransaction: function() {
    this.LOG("== UN" + this._name + " (UNAggregate) ============");
    if (this._transactions.length >= this.MIN_TRANSACTIONS_FOR_BATCH)
      this.bookmarks.beginUpdateBatch();
    for (var i = this._transactions.length - 1; i >= 0; --i) {
      var txn = this._transactions[i];
      if (this.container > -1) 
        txn.container = this.container;
      txn.undoTransaction();
    }
    if (this._transactions.length >= this.MIN_TRANSACTIONS_FOR_BATCH)
      this.bookmarks.endUpdateBatch();
    this.LOG("== UN" + this._name + " (UNAggregate Ends) =======");
  }
};


















function PlacesCreateFolderTransaction(aName, aContainer, aIndex,
                                       aAnnotations, aChildItemsTransactions) {
  this._name = aName;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._annotations = aAnnotations;
  this._id = null;
  this._childItemsTransactions = aChildItemsTransactions || [];
  this.redoTransaction = this.doTransaction;
}
PlacesCreateFolderTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  
  get container() { return this._container; },
  set container(val) { return this._container = val; },

  doTransaction: function PCFT_doTransaction() {
    var bookmarks = this.utils.bookmarks;
    this._id = bookmarks.createFolder(this._container, this._name, this._index);
    if (this._annotations.length > 0) {
      var placeURI = bookmarks.getFolderURI(this._id);
      this.utils.setAnnotationsForURI(placeURI, this._annotations);
    }
    for (var i = 0; i < this._childItemsTransactions.length; ++i) {
      var txn = this._childItemsTransactions[i];
      txn.container = this._id;
      txn.doTransaction();
    }
  },

  undoTransaction: function PCFT_undoTransaction() {
    this.bookmarks.removeFolder(this._id);
    for (var i = 0; i < this._childItemsTransactions.length; ++i) {
      var txn = this.childItemsTransactions[i];
      txn.undoTransaction();
    }
  }
};

























function PlacesCreateItemTransaction(aURI, aContainer, aIndex, aTitle,
                                     aKeyword, aAnnotations,
                                     aChildTransactions) {
  this._uri = aURI;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._title = aTitle;
  this._keyword = aKeyword;
  this._annotations = aAnnotations;
  this._childTransactions = aChildTransactions || [];
  this.redoTransaction = this.doTransaction;
}
PlacesCreateItemTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  
  get container() { return this._container; },
  set container(val) { return this._container = val; },

  doTransaction: function PCIT_doTransaction() {
    var bookmarks = this.utils.bookmarks;
    this._id = bookmarks.insertItem(this.container, this._uri, this._index);
    if (this._title)
      bookmarks.setItemTitle(this._id, this._title);
    if (this._keyword)
      bookmarks.setKeywordForBookmark(this._id, this._keyword);
    if (this._annotations && this._annotations.length > 0) {
      var placeURI = bookmarks.getItemURI(this._id);
      this.utils.setAnnotationsForURI(placeURI, this._annotations);
    }

    for (var i = 0; i < this._childTransactions.length; ++i) {
      var txn = this._childTransactions[i];
      txn.id = this._id;
      txn.doTransaction();
    }
  },

  undoTransaction: function PCIT_undoTransaction() {
    this.utils.bookmarks.removeItem(this._id);
    for (var i = 0; i < this._childTransactions.length; ++i) {
      var txn = this._childTransactions[i];
      txn.undoTransaction();
    }
  }
};











function PlacesCreateSeparatorTransaction(aContainer, aIndex) {
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._id = null;
}
PlacesCreateSeparatorTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  
  get container() { return this._container; },
  set container(val) { return this._container = val; },

  doTransaction: function PCST_doTransaction() {
    this.LOG("Create separator in: " + this.container + "," + this._index);
    this._id = this.bookmarks.insertSeparator(this.container, this._index);
  },

  undoTransaction: function PCST_undoTransaction() {
    this.LOG("UNCreate separator from: " + this.container + "," + this._index);
    this.bookmarks.removeChildAt(this.container, this._index);
  }
};
















function PlacesCreateLivemarkTransaction(aFeedURI, aSiteURI, aName,
                                         aContainer, aIndex, aAnnotations) {
  this._feedURI = aFeedURI;
  this._siteURI = aSiteURI;
  this._name = aName;
  this._container = aContainer;
  this._index = typeof(aIndex) == "number" ? aIndex : -1;
  this._annotations = aAnnotations;
}
PlacesCreateLivemarkTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  
  get container() { return this._container; },
  set container(val) { return this._container = val; },

  doTransaction: function PCLT_doTransaction() {
    this._id = this.utils.livemarks
                   .createLivemark(this._container, this._name, this._siteURI,
                                   this._feedURI, this._index);
    if (this._annotations)  {
      var placeURI = this.utils.bookmarks.getItemURI(this._id);
      this.utils.setAnnotationsForURI(placeURI, this._annotations);
    }
  },

  undoTransaction: function PCLT_undoTransaction() {
    this.bookmarks.removeFolder(this._id);
  }
};




function PlacesMoveFolderTransaction(id, oldContainer, oldIndex, newContainer, newIndex) {
  NS_ASSERT(!isNaN(id + oldContainer + oldIndex + newContainer + newIndex), "Parameter is NaN!");
  NS_ASSERT(newIndex >= -1, "invalid insertion index");
  this._id = id;
  this._oldContainer = oldContainer;
  this._oldIndex = oldIndex;
  this._newContainer = newContainer;
  this._newIndex = newIndex;
  this.redoTransaction = this.doTransaction;
}
PlacesMoveFolderTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PMFT_doTransaction() {
    this.LOG("Move Folder: " + this._id + " from: " + this._oldContainer + "," + this._oldIndex + " to: " + this._newContainer + "," + this._newIndex);
    this.bookmarks.moveFolder(this._id, this._newContainer, this._newIndex);
  },

  undoTransaction: function PMFT_undoTransaction() {
    this.LOG("UNMove Folder: " + this._id + " from: " + this._oldContainer + "," + this._oldIndex + " to: " + this._newContainer + "," + this._newIndex);
    this.bookmarks.moveFolder(this._id, this._oldContainer, this._oldIndex);
  }
};




function PlacesMoveItemTransaction(id, uri, oldContainer, oldIndex, newContainer, newIndex) {
  this._id = id;
  this._uri = uri;
  this._oldContainer = oldContainer;
  this._oldIndex = oldIndex;
  this._newContainer = newContainer;
  this._newIndex = newIndex;
  this.redoTransaction = this.doTransaction;
}
PlacesMoveItemTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PMIT_doTransaction() {
    this.LOG("Move Item: " + this._uri.spec + " from: " + this._oldContainer + "," + this._oldIndex + " to: " + this._newContainer + "," + this._newIndex);
    var title = this.bookmarks.getItemTitle(this._id);
    var placeURI = this.bookmarks.getItemURI(this._id);
    var annotations = this.utils.getAnnotationsForURI(placeURI);
    this.bookmarks.removeItem(this._id);
    this._id = this.bookmarks.insertItem(this._newContainer, this._uri, this._newIndex);
    this.bookmarks.setItemTitle(this._id, title);
    this.utils.setAnnotationsForURI(this.bookmarks.getItemURI(this._id),
                         annotations);
  },

  undoTransaction: function PMIT_undoTransaction() {
    this.LOG("UNMove Item: " + this._uri.spec + " from: " + this._oldContainer + "," + this._oldIndex + " to: " + this._newContainer + "," + this._newIndex);
    var title = this.bookmarks.getItemTitle(this._id);
    var placeURI = this.bookmarks.getItemURI(this._id);
    var annotations = this.utils.getAnnotationsForURI(placeURI);
    this.bookmarks.removeItem(this._id);
    this._id = this.bookmarks.insertItem(this._oldContainer, this._uri, this._oldIndex);
    this.bookmarks.setItemTitle(this._id, title);
    placeURI = this.bookmarks.getItemURI(this._id);
    this.utils.setAnnotationsForURI(this.bookmarks.getItemURI(this._id),
                         annotations);
  }
};










function PlacesRemoveFolderTransaction(id) {
  this._removeTxn = this.bookmarks.getRemoveFolderTransaction(id);
  this._id = id;
  this._transactions = []; 
  this.redoTransaction = this.doTransaction;
}
PlacesRemoveFolderTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype, 
  getFolderContents:
  function(aFolderId, aExcludeItems, aExpandQueries) {
    return PlacesUtils.getFolderContents(aFolderId, aExcludeItems, aExpandQueries);
  },

  



  _saveFolderContents: function PRFT__saveFolderContents() {
    this._transactions = [];
    var contents = this.getFolderContents(this._id, false, false);
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);  
    for (var i = 0; i < contents.childCount; ++i) {
      var child = contents.getChild(i);
      var txn;
      if (child.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER) {
        var folder = asFolder(child);
        txn = new PlacesRemoveFolderTransaction(folder.folderId);
      }
      else if (child.type == Ci.nsINavHistoryResultNode.RESULT_TYPE_SEPARATOR) {
        txn = new PlacesRemoveSeparatorTransaction(this._id, i);
      }
      else {
        txn = new PlacesRemoveItemTransaction(child.bookmarkId,
                                              ios.newURI(child.uri, null, null),
                                              this._id, i);
      }
      this._transactions.push(txn);
    }
  },

  doTransaction: function PRFT_doTransaction() {
    var title = this.bookmarks.getFolderTitle(this._id);
    this.LOG("Remove Folder: " + title);

    this._saveFolderContents();

    
    for (var i = this._transactions.length - 1; i >= 0; --i)
      this._transactions[i].doTransaction();
    
    
    this._removeTxn.doTransaction();
  },

  undoTransaction: function PRFT_undoTransaction() {
    this._removeTxn.undoTransaction();
    
    var title = this.bookmarks.getFolderTitle(this._id);
    this.LOG("UNRemove Folder: " + title);
    
    
    for (var i = 0; i < this._transactions.length; ++i)
      this._transactions[i].undoTransaction();
  }
};




function PlacesRemoveItemTransaction(id, uri, oldContainer, oldIndex) {
  this._id = id;
  this._uri = uri;
  this._oldContainer = oldContainer;
  this._oldIndex = oldIndex;
  this._annotations = [];
  this.redoTransaction = this.doTransaction;
}
PlacesRemoveItemTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype, 
  
  doTransaction: function PRIT_doTransaction() {
    this.LOG("Remove Item: " + this._uri.spec + " from: " + this._oldContainer + "," + this._oldIndex);
    this._title = this.bookmarks.getItemTitle(this._id);
    this._placeURI = this.bookmarks.getItemURI(this._id);
    this._annotations = this.utils.getAnnotationsForURI(this._placeURI);
    PlacesUtils.annotations.removePageAnnotations(this._placeURI);
    this.bookmarks.removeItem(this._id);
  },
  
  undoTransaction: function PRIT_undoTransaction() {
    this.LOG("UNRemove Item: " + this._uri.spec + " from: " + this._oldContainer + "," + this._oldIndex);
    this._id = this.bookmarks.insertItem(this._oldContainer, this._uri, this._oldIndex);
    this.bookmarks.setItemTitle(this._id, this._title);
    this._placeURI = this.bookmarks.getItemURI(this._id);
    this.utils.setAnnotationsForURI(this._placeURI, this._annotations);
  }
};




function PlacesRemoveSeparatorTransaction(oldContainer, oldIndex) {
  this._oldContainer = oldContainer;
  this._oldIndex = oldIndex;
  this.redoTransaction = this.doTransaction;
}
PlacesRemoveSeparatorTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype, 

  doTransaction: function PRST_doTransaction() {
    this.LOG("Remove Separator from: " + this._oldContainer + "," + this._oldIndex);
    this.bookmarks.removeChildAt(this._oldContainer, this._oldIndex);
  },
  
  undoTransaction: function PRST_undoTransaction() {
    this.LOG("UNRemove Separator from: " + this._oldContainer + "," + this._oldIndex);
    this.bookmarks.insertSeparator(this._oldContainer, this._oldIndex);
  }
};




function PlacesEditItemTitleTransaction(id, newTitle) {
  this._id = id;
  this._newTitle = newTitle;
  this._oldTitle = "";
  this.redoTransaction = this.doTransaction;
}
PlacesEditItemTitleTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PEITT_doTransaction() {
    this._oldTitle = this.bookmarks.getItemTitle(this._id);
    this.bookmarks.setItemTitle(this._id, this._newTitle);
  },

  undoTransaction: function PEITT_undoTransaction() {
    this.bookmarks.setItemTitle(this._id, this._oldTitle);
  }
};




function PlacesEditBookmarkURITransaction(aBookmarkId, aNewURI) {
  this._id = aBookmarkId;
  this._newURI = aNewURI;
  this.redoTransaction = this.doTransaction;
}
PlacesEditBookmarkURITransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PEBUT_doTransaction() {
    this._oldURI = this.bookmarks.getBookmarkURI(this._id);
    this.bookmarks.changeBookmarkURI(this._id, this._newURI);
  },

  undoTransaction: function PEBUT_undoTransaction() {
    this.bookmarks.changeBookmarkURI(this._id, this._oldURI);
  }
};




function PlacesSetLoadInSidebarTransaction(aBookmarkId, aLoadInSidebar) {
  this.id = aBookmarkId;
  this._loadInSidebar = aLoadInSidebar;
  this.redoTransaction = this.doTransaction;
}
PlacesSetLoadInSidebarTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,
  _anno: {
    name: LOAD_IN_SIDEBAR_ANNO,
    type: Ci.nsIAnnotationService.TYPE_INT32,
    value: 1,
    flags: 0,
    expires: Ci.nsIAnnotationService.EXPIRE_NEVER
  },

  doTransaction: function PSLIST_doTransaction() {
    if (!("_placeURI" in this))
      this._placeURI = this.utils.bookmarks.getItemURI(this.id);

    this._wasSet = this.utils.annotations
                       .hasAnnotation(this._placeURI, this._anno.name);
    if (this._loadInSidebar) {
      this.utils.setAnnotationsForURI(this._placeURI,
                                      [this._anno]);
    }
    else {
      try {
        this.utils.annotations.removeAnnotation(this._placeURI,
                                                this._anno.name);
      } catch(ex) { }
    }
  },

  undoTransaction: function PSLIST_undoTransaction() {
    if (this._wasSet != this._loadInSidebar) {
      this._loadInSidebar = !this._loadInSidebar;
      this.doTransaction();
    }
  }
};






function PlacesEditItemDescriptionTransaction(aBookmarkId, aDescription, aIsFolder) {
  this.id = aBookmarkId;
  this._newDescription = aDescription;
  this._isFolder = aIsFolder;
  this.redoTransaction = this.doTransaction;
}
PlacesEditItemDescriptionTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,
  _oldDescription: "",
  DESCRIPTION_ANNO: DESCRIPTION_ANNO,
  nsIAnnotationService: Components.interfaces.nsIAnnotationService,

  doTransaction: function PSLIST_doTransaction() {
    const annos = this.utils.annotations;

    if (!("_placeURI" in this)) {
      if (this._isFolder)
        this._placeURI = this.utils.bookmarks.getFolderURI(this.id);
      else
        this._placeURI = this.utils.bookmarks.getItemURI(this.id);
    }

    if (annos.hasAnnotation(this._placeURI, this.DESCRIPTION_ANNO)) {
      this._oldDescription =
        annos.getAnnotationString(this._placeURI, this.DESCRIPTION_ANNO);
    }

    if (this._newDescription) {
      annos.setAnnotationString(this._placeURI, this.DESCRIPTION_ANNO,
                                this._newDescription, 0,
                                this.nsIAnnotationService.EXPIRE_NEVER);
    }
    else if (this._oldDescription)
      annos.removeAnnotation(this._placeURI, this.DESCRIPTION_ANNO);
  },

  undoTransaction: function PSLIST_undoTransaction() {
    const annos = this.utils.annotations;

    if (this._oldDescription) {
      annos.setAnnotationString(this._placeURI, this.DESCRIPTION_ANNO,
                                this._oldDescription, 0,
                                this.nsIAnnotationService.EXPIRE_NEVER);
    }
    else if (this.utils.hasAnnotation(this._placeURI, this.DESCRIPTION_ANNO))
      annos.removeAnnotation(this._placeURI, this.DESCRIPTION_ANNO);
  }
};




function PlacesEditFolderTitleTransaction(id, newTitle) {
  this.id = id;
  this._newTitle = newTitle;
  this._oldTitle = "";
  this.redoTransaction = this.doTransaction;
}
PlacesEditFolderTitleTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PEFTT_doTransaction() {
    this._oldTitle = this.bookmarks.getFolderTitle(this.id);
    this.bookmarks.setFolderTitle(this.id, this._newTitle);
  },

  undoTransaction: function PEFTT_undoTransaction() {
    this.bookmarks.setFolderTitle(this.id, this._oldTitle);
  }
};




function PlacesEditBookmarkKeywordTransaction(id, newKeyword) {
  this.id = id;
  this._newKeyword = newKeyword;
  this._oldKeyword = "";
  this.redoTransaction = this.doTransaction;
}
PlacesEditBookmarkKeywordTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PEBKT_doTransaction() {
    this._oldKeyword = this.bookmarks.getKeywordForBookmark(this.id);
    this.bookmarks.setKeywordForBookmark(this.id, this._newKeyword);
  },

  undoTransaction: function PEBKT_undoTransaction() {
    this.bookmarks.setKeywordForBookmark(this.id, this._oldKeyword);
  }
};




function PlacesEditLivemarkSiteURITransaction(folderId, uri) {
  this._folderId = folderId;
  this._newURI = uri;
  this._oldURI = null;
  this.redoTransaction = this.doTransaction;
}
PlacesEditLivemarkSiteURITransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PELSUT_doTransaction() {
    this._oldURI = this.livemarks.getSiteURI(this._folderId);
    this.livemarks.setSiteURI(this._folderId, this._newURI);
  },

  undoTransaction: function PELSUT_undoTransaction() {
    this.livemarks.setSiteURI(this._folderId, this._oldURI);
  }
};




function PlacesEditLivemarkFeedURITransaction(folderId, uri) {
  this._folderId = folderId;
  this._newURI = uri;
  this._oldURI = null;
  this.redoTransaction = this.doTransaction;
}
PlacesEditLivemarkFeedURITransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,

  doTransaction: function PELFUT_doTransaction() {
    this._oldURI = this.livemarks.getFeedURI(this._folderId);
    this.livemarks.setFeedURI(this._folderId, this._newURI);
    this.livemarks.reloadLivemarkFolder(this._folderId);
  },

  undoTransaction: function PELFUT_undoTransaction() {
    this.livemarks.setFeedURI(this._folderId, this._oldURI);
    this.livemarks.reloadLivemarkFolder(this._folderId);
  }
};




function PlacesEditBookmarkMicrosummaryTransaction(aID, newMicrosummary) {
  this.id = aID;
  this._newMicrosummary = newMicrosummary;
  this._oldMicrosummary = null;
  this.redoTransaction = this.doTransaction;
}
PlacesEditBookmarkMicrosummaryTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,
  
  mss: Cc["@mozilla.org/microsummary/service;1"].
       getService(Ci.nsIMicrosummaryService),

  doTransaction: function PEBMT_doTransaction() {
    var placeURI = this.bookmarks.getItemURI(this.id);
    this._oldMicrosummary = this.mss.getMicrosummary(placeURI);
    if (this._newMicrosummary)
      this.mss.setMicrosummary(placeURI, this._newMicrosummary);
    else
      this.mss.removeMicrosummary(placeURI);
  },

  undoTransaction: function PEBMT_undoTransaction() {
    var placeURI = this.bookmarks.getItemURI(this.id);
    if (this._oldMicrosummary)
      this.mss.setMicrosummary(placeURI, this._oldMicrosummary);
    else
      this.mss.removeMicrosummary(placeURI);
  }
};




function PlacesSetBookmarksToolbarTransaction(aFolderId) {
  this._folderId = aFolderId;
  this._oldFolderId = this.utils.toolbarFolder;
  this.redoTransaction = this.doTransaction;
}
PlacesSetBookmarksToolbarTransaction.prototype = {
  __proto__: PlacesBaseTransaction.prototype,
  
  doTransaction: function PSBTT_doTransaction() {
    this.utils.bookmarks.toolbarFolder = this._folderId;
  },

  undoTransaction: function PSBTT_undoTransaction() {
    this.utils.bookmarks.toolbarFolder = this._oldFolderId;
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
  
#endif
}
