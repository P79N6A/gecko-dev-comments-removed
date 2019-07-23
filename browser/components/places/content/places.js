








































var PlacesOrganizer = {
  _places: null,
  _content: null,

  _initFolderTree: function() {
    var leftPaneRoot = PlacesUIUtils.leftPaneFolderId;
    this._places.place = "place:excludeItems=1&expandQueries=0&folder=" + leftPaneRoot;
  },

  selectLeftPaneQuery: function PO_selectLeftPaneQuery(aQueryName) {
    var itemId = PlacesUIUtils.leftPaneQueries[aQueryName];
    this._places.selectItems([itemId]);
    
    if (aQueryName == "AllBookmarks")
      asContainer(this._places.selectedNode).containerOpen = true;
  },

  init: function PO_init() {
    this._places = document.getElementById("placesList");
    this._content = document.getElementById("placeContent");
    this._initFolderTree();

    var leftPaneSelection = "AllBookmarks"; 
    if ("arguments" in window && window.arguments.length > 0)
      leftPaneSelection = window.arguments[0];

    this.selectLeftPaneQuery(leftPaneSelection);
    
    this._backHistory.splice(0);
    document.getElementById("OrganizerCommand:Back").setAttribute("disabled", true);

    var view = this._content.treeBoxObject.view;
    if (view.rowCount > 0)
      view.selection.select(0);

    this._content.focus();

    
    PlacesSearchBox.init();

#ifdef PLACES_QUERY_BUILDER
    
    PlacesQueryBuilder.init();
#endif

    window.addEventListener("AppCommand", this, true);
#ifdef XP_MACOSX
    
    
    var findMenuItem = document.getElementById("menu_find");
    findMenuItem.setAttribute("command", "OrganizerCommand_find:all");
    var findKey = document.getElementById("key_find");
    findKey.setAttribute("command", "OrganizerCommand_find:all");

    
    var elements = ["cmd_handleBackspace", "cmd_handleShiftBackspace"];
    for (var i=0; i < elements.length; i++) {
      document.getElementById(elements[i]).setAttribute("disabled", "true");
    }
    
    
    
    var historyMenuBack = document.getElementById("historyMenuBack");
    historyMenuBack.removeAttribute("key");
    var historyMenuForward = document.getElementById("historyMenuForward");
    historyMenuForward.removeAttribute("key");
#endif

    
    document.getElementById("placesContext")
            .removeChild(document.getElementById("placesContext_show:info"));
  },

  QueryInterface: function PO_QueryInterface(aIID) {
    if (aIID.equals(Components.interfaces.nsIDOMEventListener) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_NOINTERFACE;
  },

  handleEvent: function PO_handleEvent(aEvent) {
    if (aEvent.type != "AppCommand")
      return;

    aEvent.stopPropagation();
    switch (aEvent.command) {
      case "Back":
        if (this._backHistory.length > 0)
          this.back();
        break;
      case "Forward":
        if (this._forwardHistory.length > 0)
          this.forward();
        break;
      case "Search":
        PlacesSearchBox.findAll();
        break;
    }
  },

  destroy: function PO_destroy() {
  },

  _location: null,
  get location() {
    return this._location;
  },

  set location(aLocation) {
    if (!aLocation || this._location == aLocation)
      return aLocation;

    if (this.location) {
      this._backHistory.unshift(this.location);
      this._forwardHistory.splice(0);
    }

    this._location = aLocation;
    this._places.selectPlaceURI(aLocation);

    if (!this._places.hasSelection) {
      
      this._content.place = aLocation;
    }
    this.onContentTreeSelect();

    
    if (this._backHistory.length == 0)
      document.getElementById("OrganizerCommand:Back").setAttribute("disabled", true);
    else
      document.getElementById("OrganizerCommand:Back").removeAttribute("disabled");
    if (this._forwardHistory.length == 0)
      document.getElementById("OrganizerCommand:Forward").setAttribute("disabled", true);
    else
      document.getElementById("OrganizerCommand:Forward").removeAttribute("disabled");

    return aLocation;
  },

  _backHistory: [],
  _forwardHistory: [],

  back: function PO_back() {
    this._forwardHistory.unshift(this.location);
    var historyEntry = this._backHistory.shift();
    this._location = null;
    this.location = historyEntry;
  },
  forward: function PO_forward() {
    this._backHistory.unshift(this.location);
    var historyEntry = this._forwardHistory.shift();
    this._location = null;
    this.location = historyEntry;
  },

  









  _cachedLeftPaneSelectedURI: null,
  onPlaceSelected: function PO_onPlaceSelected(resetSearchBox) {
    
    if (!this._places.hasSelection)
      return;

    var node = this._places.selectedNode;
    var queries = asQuery(node).getQueries({});

    
    var options = node.queryOptions.clone();
    options.excludeItems = false;
    var placeURI = PlacesUtils.history.queriesToQueryString(queries,
                                                            queries.length,
                                                            options);

    
    
    
    if (this._content.place != placeURI || !resetSearchBox) {
      this._content.place = placeURI;
      PlacesSearchBox.hideSearchUI();
      this.location = node.uri;
    }

    
    
    
    
    
    var folderButton = document.getElementById("scopeBarFolder");
    var folderTitle = node.title || folderButton.getAttribute("emptytitle");
    folderButton.setAttribute("label", folderTitle);
    var cmd = document.getElementById("OrganizerCommand_find:current");
    var label = PlacesUIUtils.getFormattedString("findInPrefix", [folderTitle]);
    cmd.setAttribute("label", label);
    if (PlacesSearchBox.filterCollection == "collection")
      PlacesSearchBox.updateCollectionTitle(folderTitle);

    
    
    
    
    
    
    if (node.uri == this._cachedLeftPaneSelectedURI)
      return;
    this._cachedLeftPaneSelectedURI = node.uri;

    
    

    PlacesSearchBox.searchFilter.reset();
    this._setSearchScopeForNode(node);
    if (this._places.treeBoxObject.focused)
      this._fillDetailsPane([node]);
  },

  




  _setSearchScopeForNode: function PO__setScopeForNode(aNode) {
    var itemId = aNode.itemId;
    if (PlacesUtils.nodeIsHistoryContainer(aNode) ||
        itemId == PlacesUIUtils.leftPaneQueries["History"]) {
      PlacesQueryBuilder.setScope("history");
    }
    
    else
      PlacesQueryBuilder.setScope("bookmarks");

    
    var folderButton = document.getElementById("scopeBarFolder");
    folderButton.hidden = !PlacesUtils.nodeIsFolder(aNode) ||
                          itemId == PlacesUIUtils.allBookmarksFolderId;
  },

  






  onTreeClick: function PO_onTreeClick(aEvent) {
    
    if (aEvent.target.localName != "treechildren")
      return;

    var currentView = aEvent.currentTarget;
    var selectedNode = currentView.selectedNode;
    if (selectedNode) {
      var doubleClickOnFlatList = (aEvent.button == 0 && aEvent.detail == 2 &&
                                   aEvent.target.parentNode.flatList);
      var middleClick = (aEvent.button == 1 && aEvent.detail == 1);

      if (PlacesUtils.nodeIsURI(selectedNode) &&
          (doubleClickOnFlatList || middleClick)) {
        
        PlacesOrganizer.openSelectedNode(aEvent);
      }
      else if (middleClick &&
               PlacesUtils.nodeIsContainer(selectedNode)) {
        
        
        
        PlacesUIUtils.openContainerNodeInTabs(selectedNode, aEvent);
      }
    }
  },

  





  onTreeFocus: function PO_onTreeFocus(aEvent) {
    var currentView = aEvent.currentTarget;
    var selectedNodes = currentView.selectedNode ? [currentView.selectedNode] :
                        this._content.getSelectionNodes();
    this._fillDetailsPane(selectedNodes);
  },

  openFlatContainer: function PO_openFlatContainerFlatContainer(aContainer) {
    if (aContainer.itemId != -1)
      this._places.selectItems([aContainer.itemId]);
    else if (PlacesUtils.nodeIsQuery(aContainer))
      this._places.selectPlaceURI(aContainer.uri);
  },

  openSelectedNode: function PO_openSelectedNode(aEvent) {
    PlacesUIUtils.openNodeWithEvent(this._content.selectedNode, aEvent);
  },

  



  getCurrentOptions: function PO_getCurrentOptions() {
    return asQuery(this._content.getResult().root).queryOptions;
  },

  



  getCurrentQueries: function PO_getCurrentQueries() {
    return asQuery(this._content.getResult().root).getQueries({});
  },

  


  importBookmarks: function PO_import() {
    
    var features = "modal,centerscreen,chrome,resizable=no";

    
    
    window.fromFile = false;
    openDialog("chrome://browser/content/migration/migration.xul",
               "migration", features, "bookmarks");
    if (window.fromFile)
      this.importFromFile();
  },

  


  importFromFile: function PO_importFromFile() {
    var fp = Cc["@mozilla.org/filepicker;1"].
             createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUIUtils.getString("SelectImport"),
            Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML);
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      if (fp.file) {
        var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
                       getService(Ci.nsIPlacesImportExportService);
        var file = fp.file.QueryInterface(Ci.nsILocalFile);
        importer.importHTMLFromFile(file, false);
      }
    }
  },

  


  exportBookmarks: function PO_exportBookmarks() {
    var fp = Cc["@mozilla.org/filepicker;1"].
             createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUIUtils.getString("EnterExport"),
            Ci.nsIFilePicker.modeSave);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML);
    fp.defaultString = "bookmarks.html";
    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      var exporter = Cc["@mozilla.org/browser/places/import-export-service;1"].
                     getService(Ci.nsIPlacesImportExportService);
      exporter.exportHTMLToFile(fp.file);
    }
  },

  


  populateRestoreMenu: function PO_populateRestoreMenu() {
    var restorePopup = document.getElementById("fileRestorePopup");

    var dateSvc = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                  getService(Ci.nsIScriptableDateFormat);

    
    
    while (restorePopup.childNodes.length > 1)
      restorePopup.removeChild(restorePopup.firstChild);

    
    var localizedFilename = PlacesUtils.getString("bookmarksArchiveFilename");
    var localizedFilenamePrefix = localizedFilename.substr(0, localizedFilename.indexOf("-"));
    var fileList = [];
    var files = this.bookmarksBackupDir.directoryEntries;
    while (files.hasMoreElements()) {
      var f = files.getNext().QueryInterface(Ci.nsIFile);
      var rx = new RegExp("^(bookmarks|" + localizedFilenamePrefix +
                          ")-([0-9]{4}-[0-9]{2}-[0-9]{2})\.json$");
      if (!f.isHidden() && f.leafName.match(rx)) {
        var date = f.leafName.match(rx)[2].replace(/-/g, "/");
        var dateObj = new Date(date);
        fileList.push({date: dateObj, filename: f.leafName});
      }
    }

    fileList.sort(function PO_fileList_compare(a, b) {
      return b.date - a.date;
    });

    if (fileList.length == 0)
      return;

    
    for (var i = 0; i < fileList.length; i++) {
      var m = restorePopup.insertBefore
        (document.createElement("menuitem"),
         document.getElementById("restoreFromFile"));
      m.setAttribute("label",
                     dateSvc.FormatDate("",
                                        Ci.nsIScriptableDateFormat.dateFormatLong,
                                        fileList[i].date.getFullYear(),
                                        fileList[i].date.getMonth() + 1,
                                        fileList[i].date.getDate()));
      m.setAttribute("value", fileList[i].filename);
      m.setAttribute("oncommand",
                     "PlacesOrganizer.onRestoreMenuItemClick(this);");
    }
    restorePopup.insertBefore(document.createElement("menuseparator"),
                              document.getElementById("restoreFromFile"));
  },

  


  onRestoreMenuItemClick: function PO_onRestoreMenuItemClick(aMenuItem) {
    var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    var bookmarksFile = dirSvc.get("ProfD", Ci.nsIFile);
    bookmarksFile.append("bookmarkbackups");
    bookmarksFile.append(aMenuItem.getAttribute("value"));
    if (!bookmarksFile.exists())
      return;
    this.restoreBookmarksFromFile(bookmarksFile);
  },

  



  onRestoreBookmarksFromFile: function PO_onRestoreBookmarksFromFile() {
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUIUtils.getString("bookmarksRestoreTitle"),
            Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(PlacesUIUtils.getString("bookmarksRestoreFilterName"),
                    PlacesUIUtils.getString("bookmarksRestoreFilterExtension"));
    fp.appendFilters(Ci.nsIFilePicker.filterAll);

    var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    var backupsDir = dirSvc.get("Desk", Ci.nsILocalFile);
    fp.displayDirectory = backupsDir;

    if (fp.show() != Ci.nsIFilePicker.returnCancel)
      this.restoreBookmarksFromFile(fp.file);
  },

  


  restoreBookmarksFromFile: function PO_restoreBookmarksFromFile(aFile) {
    
    if (!aFile.leafName.match(/\.json$/)) {
      this._showErrorAlert(PlacesUIUtils.getString("bookmarksRestoreFormatError"));
      return;
    }

    
    var prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                  getService(Ci.nsIPromptService);
    if (!prompts.confirm(null,
                         PlacesUIUtils.getString("bookmarksRestoreAlertTitle"),
                         PlacesUIUtils.getString("bookmarksRestoreAlert")))
      return;

    try {
      PlacesUtils.restoreBookmarksFromJSONFile(aFile);
    }
    catch(ex) {
      this._showErrorAlert(PlacesUIUtils.getString("bookmarksRestoreParseError"));
    }
  },

  _showErrorAlert: function PO__showErrorAlert(aMsg) {
    var brandShortName = document.getElementById("brandStrings").
                                  getString("brandShortName");

    Cc["@mozilla.org/embedcomp/prompt-service;1"].
      getService(Ci.nsIPromptService).
      alert(window, brandShortName, aMsg);
  },

  




  backupBookmarks: function PO_backupBookmarks() {
    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, PlacesUIUtils.getString("bookmarksBackupTitle"),
            Ci.nsIFilePicker.modeSave);
    fp.appendFilter(PlacesUIUtils.getString("bookmarksRestoreFilterName"),
                    PlacesUIUtils.getString("bookmarksRestoreFilterExtension"));

    var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    var backupsDir = dirSvc.get("Desk", Ci.nsILocalFile);
    fp.displayDirectory = backupsDir;

    fp.defaultString = PlacesUtils.getBackupFilename();

    if (fp.show() != Ci.nsIFilePicker.returnCancel) {
      PlacesUtils.backupBookmarksToFile(fp.file);

      
      var latestBackup = PlacesUtils.getMostRecentBackup();
      if (!latestBackup || latestBackup != fp.file) {
        latestBackup.remove(false);
        var name = PlacesUtils.getBackupFilename();
        fp.file.copyTo(this.bookmarksBackupDir, name);
      }
    }
  },

  get bookmarksBackupDir() {
    delete this.bookmarksBackupDir;
    var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    var bookmarksBackupDir = dirSvc.get("ProfD", Ci.nsIFile);
    bookmarksBackupDir.append("bookmarkbackups");
    if (!bookmarksBackupDir.exists())
      bookmarksBackupDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return this.bookmarksBackupDir = bookmarksBackupDir;
  },

  _paneDisabled: false,
  _setDetailsFieldsDisabledState:
  function PO__setDetailsFieldsDisabledState(aDisabled) {
    if (aDisabled) {
      document.getElementById("paneElementsBroadcaster")
              .setAttribute("disabled", "true");
    }
    else {
      document.getElementById("paneElementsBroadcaster")
              .removeAttribute("disabled");
    }
  },

  _detectAndSetDetailsPaneMinimalState:
  function PO__detectAndSetDetailsPaneMinimalState(aNode) {
    







    var infoBox = document.getElementById("infoBox");
    var infoBoxExpander = document.getElementById("infoBoxExpander");
    var infoBoxExpanderWrapper = document.getElementById("infoBoxExpanderWrapper");

    if (!aNode) {
      infoBoxExpanderWrapper.hidden = true;
      return;
    }
    if (aNode.itemId != -1 &&
        ((PlacesUtils.nodeIsFolder(aNode) &&
          !PlacesUtils.nodeIsLivemarkContainer(aNode)) ||
         PlacesUtils.nodeIsLivemarkItem(aNode))) {
      if (infoBox.getAttribute("minimal") == "true")
        infoBox.setAttribute("wasminimal", "true");
      infoBox.removeAttribute("minimal");
      infoBoxExpanderWrapper.hidden = true;
    }
    else {
      if (infoBox.getAttribute("wasminimal") == "true")
        infoBox.setAttribute("minimal", "true");
      infoBox.removeAttribute("wasminimal");
      infoBoxExpanderWrapper.hidden = false;
    }
  },

  
  updateThumbnailProportions: function PO_updateThumbnailProportions() {
    var previewBox = document.getElementById("previewBox");
    var canvas = document.getElementById("itemThumbnail");
    var height = previewBox.boxObject.height;
    var width = height * (screen.width / screen.height);
    canvas.width = width;
    canvas.height = height;
  },

  onContentTreeSelect: function PO_onContentTreeSelect() {
    if (this._content.treeBoxObject.focused)
      this._fillDetailsPane(this._content.getSelectionNodes());
  },

  _fillDetailsPane: function PO__fillDetailsPane(aNodeList) {
    var infoBox = document.getElementById("infoBox");
    var detailsDeck = document.getElementById("detailsDeck");

    
    
    infoBox.hidden = false;
    var aSelectedNode = aNodeList.length == 1 ? aNodeList[0] : null;
    
    
    if (gEditItemOverlay.itemId != -1) {
      var focusedElement = document.commandDispatcher.focusedElement;
      if ((focusedElement instanceof HTMLInputElement ||
           focusedElement instanceof HTMLTextAreaElement) &&
          /^editBMPanel.*/.test(focusedElement.parentNode.parentNode.id))
        focusedElement.blur();

      
      
      if (aSelectedNode) {
        var concreteId = PlacesUtils.getConcreteItemId(aSelectedNode);
        var nodeIsSame = gEditItemOverlay.itemId == aSelectedNode.itemId ||
                         gEditItemOverlay.itemId == concreteId ||
                         (aSelectedNode.itemId == -1 && gEditItemOverlay.uri &&
                          gEditItemOverlay.uri == aSelectedNode.uri);
        if (nodeIsSame && detailsDeck.selectedIndex == 1 &&
            !gEditItemOverlay.multiEdit)
          return;
      }
    }

    
    gEditItemOverlay.uninitPanel(false);

    if (aSelectedNode && !PlacesUtils.nodeIsSeparator(aSelectedNode)) {
      detailsDeck.selectedIndex = 1;
      
      
      
      
      var concreteId = PlacesUtils.getConcreteItemId(aSelectedNode);
      var isRootItem = concreteId != -1 && PlacesUtils.isRootItem(concreteId);
      var readOnly = isRootItem ||
                     aSelectedNode.parent.itemId == PlacesUIUtils.leftPaneFolderId;
      var useConcreteId = isRootItem ||
                          PlacesUtils.nodeIsTagQuery(aSelectedNode);
      var itemId = -1;
      if (concreteId != -1 && useConcreteId)
        itemId = concreteId;
      else if (aSelectedNode.itemId != -1)
        itemId = aSelectedNode.itemId;
      else
        itemId = PlacesUtils._uri(aSelectedNode.uri);

      gEditItemOverlay.initPanel(itemId, { hiddenRows: ["folderPicker"],
                                           forceReadOnly: readOnly });

      
      
      
      
      
      if (aSelectedNode.itemId == -1 &&
          (PlacesUtils.nodeIsDay(aSelectedNode) ||
           PlacesUtils.nodeIsHost(aSelectedNode)))
        gEditItemOverlay._element("namePicker").value = aSelectedNode.title;

      this._detectAndSetDetailsPaneMinimalState(aSelectedNode);
    }
    else if (!aSelectedNode && aNodeList[0]) {
      var itemIds = [];
      for (var i = 0; i < aNodeList.length; i++) {
        if (!PlacesUtils.nodeIsBookmark(aNodeList[i]) &&
            !PlacesUtils.nodeIsURI(aNodeList[i])) {
          detailsDeck.selectedIndex = 0;
          var selectItemDesc = document.getElementById("selectItemDescription");
          var itemsCountLabel = document.getElementById("itemsCountText");
          selectItemDesc.hidden = false;
          itemsCountLabel.value =
            PlacesUIUtils.getFormattedString("detailsPane.multipleItems",
                                             [aNodeList.length]);
          infoBox.hidden = true;
          return;
        }
        itemIds[i] = aNodeList[i].itemId != -1 ? aNodeList[i].itemId :
                     PlacesUtils._uri(aNodeList[i].uri);
      }
      detailsDeck.selectedIndex = 1;
      gEditItemOverlay.initPanel(itemIds,
                                 { hiddenRows: ["folderPicker",
                                                "loadInSidebar",
                                                "location",
                                                "keyword",
                                                "description",
                                                "name"]});
      this._detectAndSetDetailsPaneMinimalState(aSelectedNode);
    }
    else {
      detailsDeck.selectedIndex = 0;
      infoBox.hidden = true;
      var selectItemDesc = document.getElementById("selectItemDescription");
      var itemsCountLabel = document.getElementById("itemsCountText");
      var rowCount = this._content.treeBoxObject.view.rowCount;
      if (rowCount == 0) {
        selectItemDesc.hidden = true;
        itemsCountLabel.value = PlacesUIUtils.getString("detailsPane.noItems");
      }
      else {
        selectItemDesc.hidden = false;
        if (rowCount == 1)
          itemsCountLabel.value = PlacesUIUtils.getString("detailsPane.oneItem");
        else {
          itemsCountLabel.value =
            PlacesUIUtils.getFormattedString("detailsPane.multipleItems",
                                             [rowCount]);
        }
      }
    }
  },

  
  _updateThumbnail: function PO__updateThumbnail() {
    var bo = document.getElementById("previewBox").boxObject;
    var width  = bo.width;
    var height = bo.height;

    var canvas = document.getElementById("itemThumbnail");
    var ctx = canvas.getContext('2d');
    var notAvailableText = canvas.getAttribute("notavailabletext");
    ctx.save();
    ctx.fillStyle = "-moz-Dialog";
    ctx.fillRect(0, 0, width, height);
    ctx.translate(width/2, height/2);

    ctx.fillStyle = "GrayText";
    ctx.mozTextStyle = "12pt sans serif";
    var len = ctx.mozMeasureText(notAvailableText);
    ctx.translate(-len/2,0);
    ctx.mozDrawText(notAvailableText);
    ctx.restore();
  },

  toggleAdditionalInfoFields: function PO_toggleAdditionalInfoFields() {
    var infoBox = document.getElementById("infoBox");
    var infoBoxExpander = document.getElementById("infoBoxExpander");
    var infoBoxExpanderLabel = document.getElementById("infoBoxExpanderLabel");

    if (infoBox.getAttribute("minimal") == "true") {
      infoBox.removeAttribute("minimal");
      infoBoxExpanderLabel.value = infoBoxExpanderLabel.getAttribute("lesslabel");
      infoBoxExpanderLabel.accessKey = infoBoxExpanderLabel.getAttribute("lessaccesskey");
      infoBoxExpander.className = "expander-up";
    }
    else {
      infoBox.setAttribute("minimal", "true");
      infoBoxExpanderLabel.value = infoBoxExpanderLabel.getAttribute("morelabel");
      infoBoxExpanderLabel.accessKey = infoBoxExpanderLabel.getAttribute("moreaccesskey");
      infoBoxExpander.className = "expander-down";
    }
  },

  


  saveSearch: function PO_saveSearch() {
    
    
    var options = this.getCurrentOptions();

#ifdef PLACES_QUERY_BUILDER
    var queries = PlacesQueryBuilder.queries;
#else
    var queries = this.getCurrentQueries();
#endif

    var placeSpec = PlacesUtils.history.queriesToQueryString(queries,
                                                             queries.length,
                                                             options);
    var placeURI = Cc["@mozilla.org/network/io-service;1"].
                   getService(Ci.nsIIOService).
                   newURI(placeSpec, null, null);

    
    
    
    var title = PlacesUIUtils.getString("saveSearch.title");
    var inputLabel = PlacesUIUtils.getString("saveSearch.inputLabel");
    var defaultText = PlacesUIUtils.getString("saveSearch.inputDefaultText");

    var prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                  getService(Ci.nsIPromptService);
    var check = {value: false};
    var input = {value: defaultText};
    var save = prompts.prompt(null, title, inputLabel, input, null, check);

    
    if (!save || input.value == "")
     return;

    
    var txn = PlacesUIUtils.ptm.createItem(placeURI,
                                           PlacesUtils.bookmarksMenuFolderId,
                                           PlacesUtils.bookmarks.DEFAULT_INDEX,
                                           input.value);
    PlacesUIUtils.ptm.doTransaction(txn);

    
    this._places.selectPlaceURI(placeSpec);
  }
};




var PlacesSearchBox = {

  


  get searchFilter() {
    return document.getElementById("searchFilter");
  },

  


  _folders: [],
  get folders() {
    if (this._folders.length == 0)
      this._folders.push(PlacesUtils.bookmarksMenuFolderId,
                         PlacesUtils.unfiledBookmarksFolderId,
                         PlacesUtils.toolbarFolderId);
    return this._folders;
  },
  set folders(aFolders) {
    this._folders = aFolders;
    return aFolders;
  },

  






  search: function PSB_search(filterString) {
    var PO = PlacesOrganizer;
    
    
    
    
    if (filterString == "") {
      PO.onPlaceSelected(false);
      return;
    }

    var currentOptions = PO.getCurrentOptions();
    var content = PO._content;

    
    
    switch (PlacesSearchBox.filterCollection) {
    case "collection":
      content.applyFilter(filterString, this.folders);
      
      
      
      break;
    case "bookmarks":
      content.applyFilter(filterString, this.folders);
      break;
    case "history":
      if (currentOptions.queryType != Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
        var query = PlacesUtils.history.getNewQuery();
        query.searchTerms = filterString;
        var options = currentOptions.clone();
        
        options.resultType = currentOptions.RESULT_TYPE_URI;
        options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
        content.load([query], options);
      }
      else
        content.applyFilter(filterString);
      break;
    default:
      throw "Invalid filterCollection on search";
      break;
    }

    PlacesSearchBox.showSearchUI();

    
    PlacesOrganizer.onContentTreeSelect();
  },

  


  findAll: function PSB_findAll() {
    PlacesQueryBuilder.setScope("bookmarks");
    this.focus();
  },

  


  findCurrent: function PSB_findCurrent() {
    PlacesQueryBuilder.setScope("collection");
    this.focus();
  },

  




  updateCollectionTitle: function PSB_updateCollectionTitle(title) {
    if (title)
      this.searchFilter.emptyText =
        PlacesUIUtils.getFormattedString("searchCurrentDefault", [title]);
    else
      this.searchFilter.emptyText = this.filterCollection == "history" ?
                                    PlacesUIUtils.getString("searchHistory") :
                                    PlacesUIUtils.getString("searchBookmarks");
  },

  


  get filterCollection() {
    return this.searchFilter.getAttribute("collection");
  },
  set filterCollection(collectionName) {
    if (collectionName == this.filterCollection)
      return collectionName;

    this.searchFilter.setAttribute("collection", collectionName);

    var newGrayText = null;
    if (collectionName == "collection") {
      newGrayText = PlacesOrganizer._places.selectedNode.title ||
                    document.getElementById("scopeBarFolder").
                      getAttribute("emptytitle");
    }
    this.updateCollectionTitle(newGrayText);
    return collectionName;
  },

  


  focus: function PSB_focus() {
    this.searchFilter.focus();
  },

  


  init: function PSB_init() {
    this.updateCollectionTitle();
  },

  


  get value() {
    return this.searchFilter.value;
  },
  set value(value) {
    return this.searchFilter.value = value;
  },

  showSearchUI: function PSB_showSearchUI() {
    
    var searchModifiers = document.getElementById("searchModifiers");
    searchModifiers.hidden = false;

#ifdef PLACES_QUERY_BUILDER
    
    if (PlacesQueryBuilder.numRows == 0)
      document.getElementById("OrganizerCommand_search:moreCriteria").doCommand();
#endif
  },

  hideSearchUI: function PSB_hideSearchUI() {
    var searchModifiers = document.getElementById("searchModifiers");
    searchModifiers.hidden = true;
  }
};




var PlacesQueryBuilder = {

  queries: [],
  queryOptions: null,

#ifdef PLACES_QUERY_BUILDER
  numRows: 0,

  


  _maxRows: null,

  _keywordSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectKeyword",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _locationSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectLocation",
    advancedSearch_N_LocationMenulist: "advancedSearch_N_LocationMenuSelected",
    advancedSearch_N_TimeMenulist: false,
    advancedSearch_N_Textbox: "",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: false
  },
  _timeSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: false,
    advancedSearch_N_TimePicker: "date",
    advancedSearch_N_TimeMenulist2: false
  },
  _timeInLastSearch: {
    advancedSearch_N_Subject: "advancedSearch_N_SubjectVisited",
    advancedSearch_N_LocationMenulist: false,
    advancedSearch_N_TimeMenulist: true,
    advancedSearch_N_Textbox: "7",
    advancedSearch_N_TimePicker: false,
    advancedSearch_N_TimeMenulist2: true
  },
  _nextSearch: null,
  _queryBuilders: null,


  init: function PQB_init() {
    
    this._nextSearch = {
      "keyword": this._timeSearch,
      "visited": this._locationSearch,
      "location": null
    };

    this._queryBuilders = {
      "keyword": this.setKeywordQuery,
      "visited": this.setVisitedQuery,
      "location": this.setLocationQuery
    };

    this._maxRows = this._queryBuilders.length;

    this._dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                        getService(Ci.nsIScriptableDateFormat);
  },

  


  hide: function PQB_hide() {
    var advancedSearch = document.getElementById("advancedSearch");
    
    advancedSearch.collapsed = true;
  },

  


  show: function PQB_show() {
    var advancedSearch = document.getElementById("advancedSearch");
    advancedSearch.collapsed = false;
  },

  toggleVisibility: function ABP_toggleVisibility() {
    var expander = document.getElementById("organizerScopeBarExpander");
    var advancedSearch = document.getElementById("advancedSearch");
    if (advancedSearch.collapsed) {
      advancedSearch.collapsed = false;
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
    }
    else {
      advancedSearch.collapsed = true;
      expander.className = "expander-up"
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextup"));
    }
  },

  







  _setRowId: function PQB__setRowId(element, rowId) {
    if (element.id)
      element.id = element.id.replace("advancedSearch0", "advancedSearch" + rowId);
    if (element.hasAttribute("rowid"))
      element.setAttribute("rowid", rowId);
    for (var i = 0; i < element.childNodes.length; ++i) {
      this._setRowId(element.childNodes[i], rowId);
    }
  },

  _updateUIForRowChange: function PQB__updateUIForRowChange() {
    
    
    var command = document.getElementById("OrganizerCommand_search:moreCriteria");
    if (this.numRows >= this._maxRows)
      command.setAttribute("disabled", "true");
    else
      command.removeAttribute("disabled");
  },

  



  addRow: function PQB_addRow() {
    
    
    if (this.numRows >= this._maxRows)
      return;

    
    var gridRows = document.getElementById("advancedSearchRows");
    var newRow = gridRows.firstChild.cloneNode(true);
    newRow.hidden = false;

    
    
    
    
    var searchType = this._keywordSearch;
    var lastMenu = document.getElementById("advancedSearch" +
                                           this.numRows +
                                           "Subject");
    if (this.numRows > 0 && lastMenu && lastMenu.selectedItem)
      searchType = this._nextSearch[lastMenu.selectedItem.value];

    
    if (!searchType)
      return;
    
    
    gridRows.appendChild(newRow);
    this._setRowId(newRow, ++this.numRows);

    
    
    if (this.numRows == 1) {
      this.show();

      
      
      
      
      var searchTermsField = document.getElementById("advancedSearch1Textbox");
      if (searchTermsField)
        setTimeout(function() { searchTermsField.value = PlacesSearchBox.value; }, 10);
      this.queries = PlacesOrganizer.getCurrentQueries();
      return;
    }

    this.showSearch(this.numRows, searchType);
    this._updateUIForRowChange();
  },

  





  removeRow: function PQB_removeRow(row) {
    if (!row)
      row = document.getElementById("advancedSearch" + this.numRows + "Row");
    row.parentNode.removeChild(row);
    --this.numRows;

    if (this.numRows < 1) {
      this.hide();

      
      
      
      PlacesSearchBox.search(PlacesSearchBox.value);
      return;
    }

    this.doSearch();
    this._updateUIForRowChange();
  },

  onDateTyped: function PQB_onDateTyped(event, row) {
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var dateString = textbox.value;
    var dateArr = dateString.split("-");
    
    
    
    
    var d0 = null;
    var d1 = null;
    
    
    if ((dateArr.length & 1) == 0) {
      var mid = dateArr.length / 2;
      var dateStr0 = dateArr[0];
      var dateStr1 = dateArr[mid];
      for (var i = 1; i < mid; ++i) {
        dateStr0 += "-" + dateArr[i];
        dateStr1 += "-" + dateArr[i + mid];
      }
      d0 = new Date(dateStr0);
      d1 = new Date(dateStr1);
    }
    
    if (d0 == null || d0 == "Invalid Date") {
      d0 = new Date(dateString);
    }

    if (d0 != null && d0 != "Invalid Date") {
      
      var calendar = document.getElementById("advancedSearch" + row + "Calendar");
      if (d0.getFullYear() < 2000)
        d0.setFullYear(2000 + (d0.getFullYear() % 100));
      if (d1 != null && d1 != "Invalid Date") {
        if (d1.getFullYear() < 2000)
          d1.setFullYear(2000 + (d1.getFullYear() % 100));
        calendar.updateSelection(d0, d1);
      }
      else {
        calendar.updateSelection(d0, d0);
      }

      
      this.doSearch();
    }
  },

  onCalendarChanged: function PQB_onCalendarChanged(event, row) {
    var calendar = document.getElementById("advancedSearch" + row + "Calendar");
    var begin = calendar.beginrange;
    var end = calendar.endrange;

    
    if (begin == null || end == null)
      return true;

    
    var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
    var beginDate = begin.getDate();
    var beginMonth = begin.getMonth() + 1;
    var beginYear = begin.getFullYear();
    var endDate = end.getDate();
    var endMonth = end.getMonth() + 1;
    var endYear = end.getFullYear();
    if (beginDate == endDate && beginMonth == endMonth && beginYear == endYear) {
      
      textbox.value = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
    }
    else
    {
      
      var beginStr = this._dateService.FormatDate("",
                                                   this._dateService.dateFormatShort,
                                                   beginYear,
                                                   beginMonth,
                                                   beginDate);
      var endStr = this._dateService.FormatDate("",
                                                this._dateService.dateFormatShort,
                                                endYear,
                                                endMonth,
                                                endDate);
      textbox.value = beginStr + " - " + endStr;
    }

    
    this.doSearch();

    return true;
  },

  handleTimePickerClick: function PQB_handleTimePickerClick(event, row) {
    var popup = document.getElementById("advancedSearch" + row + "DatePopup");
    if (popup.showing)
      popup.hidePopup();
    else {
      var textbox = document.getElementById("advancedSearch" + row + "TimePicker");
      popup.showPopup(textbox, -1, -1, "popup", "bottomleft", "topleft");
    }
  },

  showSearch: function PQB_showSearch(row, values) {
    for (val in values) {
      var id = val.replace("_N_", row);
      var element = document.getElementById(id);
      if (values[val] || typeof(values[val]) == "string") {
        if (typeof(values[val]) == "string") {
          if (values[val] == "date") {
            
            
            var d = new Date();
            element.value = this._dateService.FormatDate("",
                                                         this._dateService.dateFormatShort,
                                                         d.getFullYear(),
                                                         d.getMonth() + 1,
                                                         d.getDate());
            var calendar = document.getElementById("advancedSearch" + row + "Calendar");
            calendar.updateSelection(d, d);
          }
          else if (element.nodeName == "textbox") {
            
            element.value = values[val];
          } else {
            
            var itemId = values[val].replace("_N_", row);
            var item = document.getElementById(itemId);
            element.selectedItem = item;
          }
        }
        element.hidden = false;
      }
      else {
        element.hidden = true;
      }
    }

    this.doSearch();
  },

  setKeywordQuery: function PQB_setKeywordQuery(query, prefix) {
    query.searchTerms += document.getElementById(prefix + "Textbox").value + " ";
  },

  setLocationQuery: function PQB_setLocationQuery(query, prefix) {
    var type = document.getElementById(prefix + "LocationMenulist").selectedItem.value;
    if (type == "onsite") {
      query.domain = document.getElementById(prefix + "Textbox").value;
    }
    else {
      query.uriIsPrefix = (type == "startswith");
      var spec = document.getElementById(prefix + "Textbox").value;
      var ios = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);
      try {
        query.uri = ios.newURI(spec, null, null);
      }
      catch (e) {
        
        
        try {
          query.uri = ios.newURI("http://" + spec, null, null);
        }
        catch (e) {
          
          
        }
      }
    }
  },

  setVisitedQuery: function PQB_setVisitedQuery(query, prefix) {
    var searchType = document.getElementById(prefix + "TimeMenulist").selectedItem.value;
    const DAY_MSEC = 86400000;
    switch (searchType) {
      case "on":
        var calendar = document.getElementById(prefix + "Calendar");
        var begin = calendar.beginrange.getTime();
        var end = calendar.endrange.getTime();
        if (begin == end) {
          end = begin + DAY_MSEC;
        }
        query.beginTime = begin * 1000;
        query.endTime = end * 1000;
        break;
      case "before":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.beginrange.getTime();
        query.endTime = time * 1000;
        break;
      case "after":
        var calendar = document.getElementById(prefix + "Calendar");
        var time = calendar.endrange.getTime();
        query.beginTime = time * 1000;
        break;
      case "inLast":
        var textbox = document.getElementById(prefix + "Textbox");
        var amount = parseInt(textbox.value);
        amount = amount * DAY_MSEC;
        var menulist = document.getElementById(prefix + "TimeMenulist2");
        if (menulist.selectedItem.value == "weeks")
          amount = amount * 7;
        else if (menulist.selectedItem.value == "months")
          amount = amount * 30;
        var now = new Date();
        now = now - amount;
        query.beginTime = now * 1000;
        break;
    }
  },

  doSearch: function PQB_doSearch() {
    
    var queryType = document.getElementById("advancedSearchType").selectedItem.value;
    this.queries = [];
    if (queryType == "and")
      this.queries.push(PlacesUtils.history.getNewQuery());
    var updated = 0;
    for (var i = 1; updated < this.numRows; ++i) {
      var prefix = "advancedSearch" + i;

      
      
      
      var querySubjectElement = document.getElementById(prefix + "Subject");
      if (querySubjectElement) {
        
        
        var query;
        if (queryType == "and")
          query = this.queries[0];
        else
          query = PlacesUtils.history.getNewQuery();

        var querySubject = querySubjectElement.value;
        this._queryBuilders[querySubject](query, prefix);

        if (queryType == "or")
          this.queries.push(query);

        ++updated;
      }
    }

    
    this.options = PlacesOrganizer.getCurrentOptions();
    this.options.resultType = this.options.RESULT_TYPE_URI;

    
    PlacesOrganizer._content.load(this.queries, this.options);

    
    PlacesOrganizer.onContentTreeSelect();
  },
#endif

  




  onScopeSelected: function PQB_onScopeSelected(aButton) {
    switch (aButton.id) {
    case "scopeBarHistory":
      this.setScope("history");
      break;
    case "scopeBarFolder":
      this.setScope("collection");
      break;
    case "scopeBarAll":
      this.setScope("bookmarks");
      break;
    default:
      throw "Invalid search scope button ID";
      break;
    }
  },

  







  setScope: function PQB_setScope(aScope) {
    
    var filterCollection;
    var folders = [];
    var scopeButtonId;
    switch (aScope) {
    case "history":
      filterCollection = "history";
      scopeButtonId = "scopeBarHistory";
      break;
    case "collection":
      
      
      
      if (!document.getElementById("scopeBarFolder").hidden) {
        filterCollection = "collection";
        scopeButtonId = "scopeBarFolder";
        folders.push(PlacesUtils.getConcreteItemId(
                       PlacesOrganizer._places.selectedNode));
        break;
      }
      
      
    case "bookmarks":
      filterCollection = "bookmarks";
      scopeButtonId = "scopeBarAll";
      folders.push(PlacesUtils.bookmarksMenuFolderId,
                   PlacesUtils.toolbarFolderId,
                   PlacesUtils.unfiledBookmarksFolderId);
      break;
    default:
      throw "Invalid search scope";
      break;
    }

    
    document.getElementById(scopeButtonId).checked = true;

    
    PlacesSearchBox.filterCollection = filterCollection;
    PlacesSearchBox.folders = folders;
    var searchStr = PlacesSearchBox.searchFilter.value;
    if (searchStr)
      PlacesSearchBox.search(searchStr);
  }
};




var ViewMenu = {
  



















  _clean: function VM__clean(popup, startID, endID) {
    if (endID)
      NS_ASSERT(startID, "meaningless to have valid endID and null startID");
    if (startID) {
      var startElement = document.getElementById(startID);
      NS_ASSERT(startElement.parentNode ==
                popup, "startElement is not in popup");
      NS_ASSERT(startElement,
                "startID does not correspond to an existing element");
      var endElement = null;
      if (endID) {
        endElement = document.getElementById(endID);
        NS_ASSERT(endElement.parentNode == popup,
                  "endElement is not in popup");
        NS_ASSERT(endElement,
                  "endID does not correspond to an existing element");
      }
      while (startElement.nextSibling != endElement)
        popup.removeChild(startElement.nextSibling);
      return endElement;
    }
    else {
      while(popup.hasChildNodes())
        popup.removeChild(popup.firstChild);
    }
    return null;
  },

  




















  fillWithColumns: function VM_fillWithColumns(event, startID, endID, type, propertyPrefix) {
    var popup = event.target;
    var pivot = this._clean(popup, startID, endID);

    
    
    var isSorted = false;
    var content = document.getElementById("placeContent");
    var columns = content.columns;
    for (var i = 0; i < columns.count; ++i) {
      var column = columns.getColumnAt(i).element;
      var menuitem = document.createElement("menuitem");
      menuitem.id = "menucol_" + column.id;
      menuitem.column = column;
      var label = column.getAttribute("label");
      if (propertyPrefix) {
        var menuitemPrefix = propertyPrefix;
        
        
        var columnId = column.getAttribute("anonid");
        menuitemPrefix += columnId == "title" ? "name" : columnId;
        label = PlacesUIUtils.getString(menuitemPrefix + ".label");
        var accesskey = PlacesUIUtils.getString(menuitemPrefix + ".accesskey");
        menuitem.setAttribute("accesskey", accesskey);
      }
      menuitem.setAttribute("label", label);
      if (type == "radio") {
        menuitem.setAttribute("type", "radio");
        menuitem.setAttribute("name", "columns");
        
        if (column.getAttribute("sortDirection") != "") {
          menuitem.setAttribute("checked", "true");
          isSorted = true;
        }
      }
      else if (type == "checkbox") {
        menuitem.setAttribute("type", "checkbox");
        
        if (column.getAttribute("primary") == "true")
          menuitem.setAttribute("disabled", "true");
        
        if (!column.hidden)
          menuitem.setAttribute("checked", "true");
      }
      if (pivot)
        popup.insertBefore(menuitem, pivot);
      else
        popup.appendChild(menuitem);
    }
    event.stopPropagation();
  },

  


  populateSortMenu: function VM_populateSortMenu(event) {
    this.fillWithColumns(event, "viewUnsorted", "directionSeparator", "radio", "view.sortBy.");

    var sortColumn = this._getSortColumn();
    var viewSortAscending = document.getElementById("viewSortAscending");
    var viewSortDescending = document.getElementById("viewSortDescending");
    
    
    var viewUnsorted = document.getElementById("viewUnsorted");
    if (!sortColumn) {
      viewSortAscending.removeAttribute("checked");
      viewSortDescending.removeAttribute("checked");
      viewUnsorted.setAttribute("checked", "true");
    }
    else if (sortColumn.getAttribute("sortDirection") == "ascending") {
      viewSortAscending.setAttribute("checked", "true");
      viewSortDescending.removeAttribute("checked");
      viewUnsorted.removeAttribute("checked");
    }
    else if (sortColumn.getAttribute("sortDirection") == "descending") {
      viewSortDescending.setAttribute("checked", "true");
      viewSortAscending.removeAttribute("checked");
      viewUnsorted.removeAttribute("checked");
    }
  },

  




  showHideColumn: function VM_showHideColumn(element) {
    var column = element.column;

    var splitter = column.nextSibling;
    if (splitter && splitter.localName != "splitter")
      splitter = null;

    if (element.getAttribute("checked") == "true") {
      column.setAttribute("hidden", "false");
      if (splitter)
        splitter.removeAttribute("hidden");
    }
    else {
      column.setAttribute("hidden", "true");
      if (splitter)
        splitter.setAttribute("hidden", "true");
    }
  },

  



  _getSortColumn: function VM__getSortColumn() {
    var content = document.getElementById("placeContent");
    var cols = content.columns;
    for (var i = 0; i < cols.count; ++i) {
      var column = cols.getColumnAt(i).element;
      var sortDirection = column.getAttribute("sortDirection");
      if (sortDirection == "ascending" || sortDirection == "descending")
        return column;
    }
    return null;
  },

  










  setSortColumn: function VM_setSortColumn(aColumn, aDirection) {
    var result = document.getElementById("placeContent").getResult();
    if (!aColumn && !aDirection) {
      result.sortingMode = Ci.nsINavHistoryQueryOptions.SORT_BY_NONE;
      return;
    }

    var columnId;
    if (aColumn) {
      columnId = aColumn.getAttribute("anonid");
      if (!aDirection) {
        var sortColumn = this._getSortColumn();
        if (sortColumn)
          aDirection = sortColumn.getAttribute("sortDirection");
      }
    }
    else {
      var sortColumn = this._getSortColumn();
      columnId = sortColumn ? sortColumn.getAttribute("anonid") : "title";
    }

    
    
    
    
    
    
    
    var colLookupTable = {
      title:        { key: "TITLE",        dir: "ascending"  },
      tags:         { key: "TAGS",         dir: "ascending"  },
      url:          { key: "URI",          dir: "ascending"  },
      date:         { key: "DATE",         dir: "descending" },
      visitCount:   { key: "VISITCOUNT",   dir: "descending" },
      keyword:      { key: "KEYWORD",      dir: "ascending"  },
      dateAdded:    { key: "DATEADDED",    dir: "descending" },
      lastModified: { key: "LASTMODIFIED", dir: "descending" },
      description:  { key: "ANNOTATION",
                      dir: "ascending",
                      anno: DESCRIPTION_ANNO }
    };

    
    if (!colLookupTable.hasOwnProperty(columnId))
      throw("Invalid column");

    
    
    
    aDirection = (aDirection || colLookupTable[columnId].dir).toUpperCase();

    var sortConst = "SORT_BY_" + colLookupTable[columnId].key + "_" + aDirection;
    result.sortingAnnotation = colLookupTable[columnId].anno || "";
    result.sortingMode = Ci.nsINavHistoryQueryOptions[sortConst];
  }
};
