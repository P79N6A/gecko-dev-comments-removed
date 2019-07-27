




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MigrationUtils",
                                  "resource:///modules/MigrationUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "BookmarkJSONUtils",
                                  "resource://gre/modules/BookmarkJSONUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
                                  "resource://gre/modules/PlacesBackups.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");

const RESTORE_FILEPICKER_FILTER_EXT = "*.json;*.jsonlz4";
const HISTORY_LIBRARY_SEARCH_TELEMETRY = "PLACES_HISTORY_LIBRARY_SEARCH_TIME_MS";

var PlacesOrganizer = {
  _places: null,

  
  
  
  _additionalInfoFields: [
    "editBMPanel_descriptionRow",
    "editBMPanel_loadInSidebarCheckbox",
    "editBMPanel_keywordRow",
  ],

  _initFolderTree: function() {
    var leftPaneRoot = PlacesUIUtils.leftPaneFolderId;
    this._places.place = "place:excludeItems=1&expandQueries=0&folder=" + leftPaneRoot;
  },

  selectLeftPaneQuery: function PO_selectLeftPaneQuery(aQueryName) {
    var itemId = PlacesUIUtils.leftPaneQueries[aQueryName];
    this._places.selectItems([itemId]);
    
    if (aQueryName == "AllBookmarks" || aQueryName == "History")
      PlacesUtils.asContainer(this._places.selectedNode).containerOpen = true;
  },

  









  selectLeftPaneContainerByHierarchy:
  function PO_selectLeftPaneContainerByHierarchy(aHierarchy) {
    if (!aHierarchy)
      throw new Error("Invalid containers hierarchy");
    let hierarchy = [].concat(aHierarchy);
    let selectWasSuppressed = this._places.view.selection.selectEventsSuppressed;
    if (!selectWasSuppressed)
      this._places.view.selection.selectEventsSuppressed = true;
    try {
      for (let container of hierarchy) {
        switch (typeof container) {
          case "number":
            this._places.selectItems([container], false);
            break;
          case "string":
            if (container.substr(0, 6) == "place:")
              this._places.selectPlaceURI(container);
            else if (container in PlacesUIUtils.leftPaneQueries)
              this.selectLeftPaneQuery(container);
            else
              throw new Error("Invalid container found: " + container);
            break;
          default:
            throw new Error("Invalid container type found: " + container);
            break;
        }
        PlacesUtils.asContainer(this._places.selectedNode).containerOpen = true;
      }
    } finally {
      if (!selectWasSuppressed)
        this._places.view.selection.selectEventsSuppressed = false;
    }
  },

  init: function PO_init() {
    ContentArea.init();

    this._places = document.getElementById("placesList");
    this._initFolderTree();

    var leftPaneSelection = "AllBookmarks"; 
    if (window.arguments && window.arguments[0])
      leftPaneSelection = window.arguments[0];

    this.selectLeftPaneContainerByHierarchy(leftPaneSelection);
    if (leftPaneSelection === "History") {
      let historyNode = this._places.selectedNode;
      if (historyNode.childCount > 0)
        this._places.selectNode(historyNode.getChild(0));
    }

    
    this._backHistory.splice(0, this._backHistory.length);
    document.getElementById("OrganizerCommand:Back").setAttribute("disabled", true);

    
    PlacesSearchBox.init();

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
#endif

    
    document.getElementById("placesContext")
            .removeChild(document.getElementById("placesContext_show:info"));

    ContentArea.focus();
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
      this._forwardHistory.splice(0, this._forwardHistory.length);
    }

    this._location = aLocation;
    this._places.selectPlaceURI(aLocation);

    if (!this._places.hasSelection) {
      
      ContentArea.currentPlace = aLocation;
    }
    this.updateDetailsPane();

    
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
    var queries = PlacesUtils.asQuery(node).getQueries();

    
    var options = node.queryOptions.clone();
    options.excludeItems = false;
    var placeURI = PlacesUtils.history.queriesToQueryString(queries,
                                                            queries.length,
                                                            options);

    
    
    
    if (ContentArea.currentPlace != placeURI || !resetSearchBox) {
      ContentArea.currentPlace = placeURI;
      this.location = node.uri;
    }

    
    
    
    
    
    
    if (node.uri == this._cachedLeftPaneSelectedURI)
      return;
    this._cachedLeftPaneSelectedURI = node.uri;

    
    

    PlacesSearchBox.searchFilter.reset();
    this._setSearchScopeForNode(node);
    this.updateDetailsPane();
  },

  




  _setSearchScopeForNode: function PO__setScopeForNode(aNode) {
    let itemId = aNode.itemId;

    if (PlacesUtils.nodeIsHistoryContainer(aNode) ||
        itemId == PlacesUIUtils.leftPaneQueries["History"]) {
      PlacesQueryBuilder.setScope("history");
    }
    else if (itemId == PlacesUIUtils.leftPaneQueries["Downloads"]) {
      PlacesQueryBuilder.setScope("downloads");
    }
    else {
      
      PlacesQueryBuilder.setScope("bookmarks");
    }
  },

  






  onPlacesListClick: function PO_onPlacesListClick(aEvent) {
    
    if (aEvent.target.localName != "treechildren")
      return;

    let node = this._places.selectedNode;
    if (node) {
      let middleClick = aEvent.button == 1 && aEvent.detail == 1;
      if (middleClick && PlacesUtils.nodeIsContainer(node)) {
        
        
        
        PlacesUIUtils.openContainerNodeInTabs(selectedNode, aEvent, this._places);
      }
    }
  },

  


  updateDetailsPane: function PO_updateDetailsPane() {
    if (!ContentArea.currentViewOptions.showDetailsPane)
      return;
    let view = PlacesUIUtils.getViewForNode(document.activeElement);
    if (view) {
      let selectedNodes = view.selectedNode ?
                          [view.selectedNode] : view.selectedNodes;
      this._fillDetailsPane(selectedNodes);
    }
  },

  openFlatContainer: function PO_openFlatContainerFlatContainer(aContainer) {
    if (aContainer.itemId != -1) {
      PlacesUtils.asContainer(this._places.selectedNode).containerOpen = true;
      this._places.selectItems([aContainer.itemId], false);
    }
    else if (PlacesUtils.nodeIsQuery(aContainer)) {
      this._places.selectPlaceURI(aContainer.uri);
    }
  },

  



  getCurrentOptions: function PO_getCurrentOptions() {
    return PlacesUtils.asQuery(ContentArea.currentView.result.root).queryOptions;
  },

  



  getCurrentQueries: function PO_getCurrentQueries() {
    return PlacesUtils.asQuery(ContentArea.currentView.result.root).getQueries();
  },

  



  importFromBrowser: function PO_importFromBrowser() {
    MigrationUtils.showMigrationWizard(window);
  },

  


  importFromFile: function PO_importFromFile() {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult != Ci.nsIFilePicker.returnCancel && fp.fileURL) {
        Components.utils.import("resource://gre/modules/BookmarkHTMLUtils.jsm");
        BookmarkHTMLUtils.importFromURL(fp.fileURL.spec, false)
                         .then(null, Components.utils.reportError);
      }
    };

    fp.init(window, PlacesUIUtils.getString("SelectImport"),
            Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML);
    fp.open(fpCallback);
  },

  


  exportBookmarks: function PO_exportBookmarks() {
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult != Ci.nsIFilePicker.returnCancel) {
        Components.utils.import("resource://gre/modules/BookmarkHTMLUtils.jsm");
        BookmarkHTMLUtils.exportToFile(fp.file.path)
                         .then(null, Components.utils.reportError);
      }
    };

    fp.init(window, PlacesUIUtils.getString("EnterExport"),
            Ci.nsIFilePicker.modeSave);
    fp.appendFilters(Ci.nsIFilePicker.filterHTML);
    fp.defaultString = "bookmarks.html";
    fp.open(fpCallback);
  },

  


  populateRestoreMenu: function PO_populateRestoreMenu() {
    let restorePopup = document.getElementById("fileRestorePopup");

    let dateSvc = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                  getService(Ci.nsIScriptableDateFormat);

    
    while (restorePopup.childNodes.length > 1)
      restorePopup.removeChild(restorePopup.firstChild);

    Task.spawn(function() {
      let backupFiles = yield PlacesBackups.getBackupFiles();
      if (backupFiles.length == 0)
        return;

      
      for (let i = 0; i < backupFiles.length; i++) {
        let fileSize = (yield OS.File.stat(backupFiles[i])).size;
        let [size, unit] = DownloadUtils.convertByteUnits(fileSize);
        let sizeString = PlacesUtils.getFormattedString("backupFileSizeText",
                                                        [size, unit]);
        let sizeInfo;
        let bookmarkCount = PlacesBackups.getBookmarkCountForFile(backupFiles[i]);
        if (bookmarkCount != null) {
          sizeInfo = " (" + sizeString + " - " +
                     PlacesUIUtils.getPluralString("detailsPane.itemsCountLabel",
                                                   bookmarkCount,
                                                   [bookmarkCount]) +
                     ")";
        } else {
          sizeInfo = " (" + sizeString + ")";
        }

        let backupDate = PlacesBackups.getDateForFile(backupFiles[i]);
        let m = restorePopup.insertBefore(document.createElement("menuitem"),
                                          document.getElementById("restoreFromFile"));
        m.setAttribute("label",
                       dateSvc.FormatDate("",
                                          Ci.nsIScriptableDateFormat.dateFormatLong,
                                          backupDate.getFullYear(),
                                          backupDate.getMonth() + 1,
                                          backupDate.getDate()) +
                                          sizeInfo);
        m.setAttribute("value", OS.Path.basename(backupFiles[i]));
        m.setAttribute("oncommand",
                       "PlacesOrganizer.onRestoreMenuItemClick(this);");
      }

      
      restorePopup.insertBefore(document.createElement("menuseparator"),
                                document.getElementById("restoreFromFile"));
    });
  },

  


  onRestoreMenuItemClick: function PO_onRestoreMenuItemClick(aMenuItem) {
    Task.spawn(function() {
      let backupName = aMenuItem.getAttribute("value");
      let backupFilePaths = yield PlacesBackups.getBackupFiles();
      for (let backupFilePath of backupFilePaths) {
        if (OS.Path.basename(backupFilePath) == backupName) {
          PlacesOrganizer.restoreBookmarksFromFile(backupFilePath);
          break;
        }
      }
    });
  },

  



  onRestoreBookmarksFromFile: function PO_onRestoreBookmarksFromFile() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    let backupsDir = dirSvc.get("Desk", Ci.nsILocalFile);
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult != Ci.nsIFilePicker.returnCancel) {
        this.restoreBookmarksFromFile(fp.file.path);
      }
    }.bind(this);

    fp.init(window, PlacesUIUtils.getString("bookmarksRestoreTitle"),
            Ci.nsIFilePicker.modeOpen);
    fp.appendFilter(PlacesUIUtils.getString("bookmarksRestoreFilterName"),
                    RESTORE_FILEPICKER_FILTER_EXT);
    fp.appendFilters(Ci.nsIFilePicker.filterAll);
    fp.displayDirectory = backupsDir;
    fp.open(fpCallback);
  },

  


  restoreBookmarksFromFile: function PO_restoreBookmarksFromFile(aFilePath) {
    
    if (!aFilePath.endsWith("json") && !aFilePath.endsWith("jsonlz4"))  {
      this._showErrorAlert(PlacesUIUtils.getString("bookmarksRestoreFormatError"));
      return;
    }

    
    var prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                  getService(Ci.nsIPromptService);
    if (!prompts.confirm(null,
                         PlacesUIUtils.getString("bookmarksRestoreAlertTitle"),
                         PlacesUIUtils.getString("bookmarksRestoreAlert")))
      return;

    Task.spawn(function() {
      try {
        yield BookmarkJSONUtils.importFromFile(aFilePath, true);
      } catch(ex) {
        PlacesOrganizer._showErrorAlert(PlacesUIUtils.getString("bookmarksRestoreParseError"));
      }
    });
  },

  _showErrorAlert: function PO__showErrorAlert(aMsg) {
    var brandShortName = document.getElementById("brandStrings").
                                  getString("brandShortName");

    Cc["@mozilla.org/embedcomp/prompt-service;1"].
      getService(Ci.nsIPromptService).
      alert(window, brandShortName, aMsg);
  },

  




  backupBookmarks: function PO_backupBookmarks() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    let backupsDir = dirSvc.get("Desk", Ci.nsILocalFile);
    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let fpCallback = function fpCallback_done(aResult) {
      if (aResult != Ci.nsIFilePicker.returnCancel) {
        
        PlacesBackups.saveBookmarksToJSONFile(fp.file.path);
      }
    };

    fp.init(window, PlacesUIUtils.getString("bookmarksBackupTitle"),
            Ci.nsIFilePicker.modeSave);
    fp.appendFilter(PlacesUIUtils.getString("bookmarksRestoreFilterName"),
                    RESTORE_FILEPICKER_FILTER_EXT);
    fp.defaultString = PlacesBackups.getFilenameForDate();
    fp.displayDirectory = backupsDir;
    fp.open(fpCallback);
  },

  _detectAndSetDetailsPaneMinimalState:
  function PO__detectAndSetDetailsPaneMinimalState(aNode) {
    







    var infoBox = document.getElementById("infoBox");
    var infoBoxExpander = document.getElementById("infoBoxExpander");
    var infoBoxExpanderWrapper = document.getElementById("infoBoxExpanderWrapper");
    var additionalInfoBroadcaster = document.getElementById("additionalInfoBroadcaster");

    if (!aNode) {
      infoBoxExpanderWrapper.hidden = true;
      return;
    }
    if (aNode.itemId != -1 &&
        PlacesUtils.nodeIsFolder(aNode) && !aNode._feedURI) {
      if (infoBox.getAttribute("minimal") == "true")
        infoBox.setAttribute("wasminimal", "true");
      infoBox.removeAttribute("minimal");
      infoBoxExpanderWrapper.hidden = true;
    }
    else {
      if (infoBox.getAttribute("wasminimal") == "true")
        infoBox.setAttribute("minimal", "true");
      infoBox.removeAttribute("wasminimal");
      infoBoxExpanderWrapper.hidden =
        this._additionalInfoFields.every(function (id)
          document.getElementById(id).collapsed);
    }
    additionalInfoBroadcaster.hidden = infoBox.getAttribute("minimal") == "true";
  },

  
  updateThumbnailProportions: function PO_updateThumbnailProportions() {
    var previewBox = document.getElementById("previewBox");
    var canvas = document.getElementById("itemThumbnail");
    var height = previewBox.boxObject.height;
    var width = height * (screen.width / screen.height);
    canvas.width = width;
    canvas.height = height;
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

      gEditItemOverlay.initPanel(itemId, { hiddenRows: ["folderPicker"]
                                         , forceReadOnly: readOnly
                                         , titleOverride: aSelectedNode.title
                                         });

      
      
      
      
      
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
            PlacesUIUtils.getPluralString("detailsPane.itemsCountLabel",
                                          aNodeList.length, [aNodeList.length]);
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
      let selectItemDesc = document.getElementById("selectItemDescription");
      let itemsCountLabel = document.getElementById("itemsCountText");
      let itemsCount = 0;
      if (ContentArea.currentView.result) {
        let rootNode = ContentArea.currentView.result.root;
        if (rootNode.containerOpen)
          itemsCount = rootNode.childCount;
      }
      if (itemsCount == 0) {
        selectItemDesc.hidden = true;
        itemsCountLabel.value = PlacesUIUtils.getString("detailsPane.noItems");
      }
      else {
        selectItemDesc.hidden = false;
        itemsCountLabel.value =
          PlacesUIUtils.getPluralString("detailsPane.itemsCountLabel",
                                        itemsCount, [itemsCount]);
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
    var additionalInfoBroadcaster = document.getElementById("additionalInfoBroadcaster");

    if (infoBox.getAttribute("minimal") == "true") {
      infoBox.removeAttribute("minimal");
      infoBoxExpanderLabel.value = infoBoxExpanderLabel.getAttribute("lesslabel");
      infoBoxExpanderLabel.accessKey = infoBoxExpanderLabel.getAttribute("lessaccesskey");
      infoBoxExpander.className = "expander-up";
      additionalInfoBroadcaster.removeAttribute("hidden");
    }
    else {
      infoBox.setAttribute("minimal", "true");
      infoBoxExpanderLabel.value = infoBoxExpanderLabel.getAttribute("morelabel");
      infoBoxExpanderLabel.accessKey = infoBoxExpanderLabel.getAttribute("moreaccesskey");
      infoBoxExpander.className = "expander-down";
      additionalInfoBroadcaster.setAttribute("hidden", "true");
    }
  },
};




var PlacesSearchBox = {

  


  get searchFilter() {
    return document.getElementById("searchFilter");
  },
   
  


  _folders: [],
  get folders() {
    if (this._folders.length == 0) {
      this._folders.push(PlacesUtils.bookmarksMenuFolderId,
                         PlacesUtils.unfiledBookmarksFolderId,
                         PlacesUtils.toolbarFolderId);
    }
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

    let currentView = ContentArea.currentView;
    let currentOptions = PO.getCurrentOptions();

    
    
    switch (PlacesSearchBox.filterCollection) {
      case "bookmarks":
        currentView.applyFilter(filterString, this.folders);
        break;
      case "history":
        if (currentOptions.queryType != Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY) {
          var query = PlacesUtils.history.getNewQuery();
          query.searchTerms = filterString;
          var options = currentOptions.clone();
          
          options.resultType = currentOptions.RESULTS_AS_URI;
          options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
          options.includeHidden = true;
          currentView.load([query], options);
        }
        else {
          TelemetryStopwatch.start(HISTORY_LIBRARY_SEARCH_TELEMETRY);
          currentView.applyFilter(filterString, null, true);
          TelemetryStopwatch.finish(HISTORY_LIBRARY_SEARCH_TELEMETRY);
        }
        break;
      case "downloads":
        if (currentView == ContentTree.view) {
          let query = PlacesUtils.history.getNewQuery();
          query.searchTerms = filterString;
          query.setTransitions([Ci.nsINavHistoryService.TRANSITION_DOWNLOAD], 1);
          let options = currentOptions.clone();
          
          options.resultType = currentOptions.RESULTS_AS_URI;
          options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
          options.includeHidden = true;
          currentView.load([query], options);
        }
        else {
          
          currentView.searchTerm = filterString;
        }
        break;
      default:
        throw "Invalid filterCollection on search";
    }

    
    PlacesOrganizer.updateDetailsPane();
  },

  


  findAll: function PSB_findAll() {
    switch (this.filterCollection) {
      case "history":
        PlacesQueryBuilder.setScope("history");
        break;
      case "downloads":
        PlacesQueryBuilder.setScope("downloads");
        break;
      default:
        PlacesQueryBuilder.setScope("bookmarks");
        break;
    }
    this.focus();
  },

  




  updateCollectionTitle: function PSB_updateCollectionTitle(aTitle) {
    let title = "";
    switch (this.filterCollection) {
      case "history":
        title = PlacesUIUtils.getString("searchHistory");
        break;
      case "downloads":
        title = PlacesUIUtils.getString("searchDownloads");
        break;
      default:
        title = PlacesUIUtils.getString("searchBookmarks");                                    
    }
    this.searchFilter.placeholder = title;
  },

  


  get filterCollection() {
    return this.searchFilter.getAttribute("collection");
  },
  set filterCollection(collectionName) {
    if (collectionName == this.filterCollection)
      return collectionName;

    this.searchFilter.setAttribute("collection", collectionName);
    this.updateCollectionTitle();

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
};




var PlacesQueryBuilder = {

  queries: [],
  queryOptions: null,

  








  setScope: function PQB_setScope(aScope) {
    
    var filterCollection;
    var folders = [];
    switch (aScope) {
      case "history":
        filterCollection = "history";
        break;
      case "bookmarks":
        filterCollection = "bookmarks";
        folders.push(PlacesUtils.bookmarksMenuFolderId,
                     PlacesUtils.toolbarFolderId,
                     PlacesUtils.unfiledBookmarksFolderId);
        break;
      case "downloads":
        filterCollection = "downloads";
        break;
      default:
        throw "Invalid search scope";
        break;
    }

    
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
    this.fillWithColumns(event, "viewUnsorted", "directionSeparator", "radio", "view.sortBy.1.");

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
    var result = document.getElementById("placeContent").result;
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
                      anno: PlacesUIUtils.DESCRIPTION_ANNO }
    };

    
    if (!colLookupTable.hasOwnProperty(columnId))
      throw("Invalid column");

    
    
    
    aDirection = (aDirection || colLookupTable[columnId].dir).toUpperCase();

    var sortConst = "SORT_BY_" + colLookupTable[columnId].key + "_" + aDirection;
    result.sortingAnnotation = colLookupTable[columnId].anno || "";
    result.sortingMode = Ci.nsINavHistoryQueryOptions[sortConst];
  }
}

let ContentArea = {
  _specialViews: new Map(),

  init: function CA_init() {
    this._deck = document.getElementById("placesViewsDeck");
    this._toolbar = document.getElementById("placesToolbar");
    ContentTree.init();
    this._setupView();
  },

  








  getContentViewForQueryString:
  function CA_getContentViewForQueryString(aQueryString) {
    try {
      if (this._specialViews.has(aQueryString)) {
        let { view, options } = this._specialViews.get(aQueryString);
        if (typeof view == "function") {
          view = view();
          this._specialViews.set(aQueryString, { view: view, options: options });
        }
        return view;
      }
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
    return ContentTree.view;
  },

  











  setContentViewForQueryString:
  function CA_setContentViewForQueryString(aQueryString, aView, aOptions) {
    if (!aQueryString ||
        typeof aView != "object" && typeof aView != "function")
      throw new Error("Invalid arguments");

    this._specialViews.set(aQueryString, { view: aView,
                                           options: aOptions || new Object() });
  },

  get currentView() PlacesUIUtils.getViewForNode(this._deck.selectedPanel),
  set currentView(aNewView) {
    let oldView = this.currentView;
    if (oldView != aNewView) {
      this._deck.selectedPanel = aNewView.associatedElement;

      
      
      if (document.activeElement == oldView.associatedElement)
        aNewView.associatedElement.focus();
    }
    return aNewView;
  },

  get currentPlace() this.currentView.place,
  set currentPlace(aQueryString) {
    let oldView = this.currentView;
    let newView = this.getContentViewForQueryString(aQueryString);
    newView.place = aQueryString;
    if (oldView != newView) {
      oldView.active = false;
      this.currentView = newView;
      this._setupView();
      newView.active = true;
    }
    return aQueryString;
  },

  


  _setupView: function CA__setupView() {
    let options = this.currentViewOptions;

    
    let detailsDeck = document.getElementById("detailsDeck");
    detailsDeck.hidden = !options.showDetailsPane;

    
    for (let elt of this._toolbar.childNodes) {
      
      if (elt.id == "placesMenu") {
        for (let menuElt of elt.childNodes) {
          menuElt.hidden = options.toolbarSet.indexOf(menuElt.id) == -1;
        }
      }
      else {
        elt.hidden = options.toolbarSet.indexOf(elt.id) == -1;
      }
    }
  },

  




  get currentViewOptions() {
    
    let viewOptions = ContentTree.viewOptions;
    if (this._specialViews.has(this.currentPlace)) {
      let { view, options } = this._specialViews.get(this.currentPlace);
      for (let option in options) {
        viewOptions[option] = options[option];
      }
    }
    return viewOptions;
  },

  focus: function() {
    this._deck.selectedPanel.focus();
  }
};

let ContentTree = {
  init: function CT_init() {
    this._view = document.getElementById("placeContent");
  },

  get view() this._view,

  get viewOptions() Object.seal({
    showDetailsPane: true,
    toolbarSet: "back-button, forward-button, organizeButton, viewMenu, maintenanceButton, libraryToolbarSpacer, searchFilter"
  }),

  openSelectedNode: function CT_openSelectedNode(aEvent) {
    let view = this.view;
    PlacesUIUtils.openNodeWithEvent(view.selectedNode, aEvent, view);
  },

  onClick: function CT_onClick(aEvent) {
    let node = this.view.selectedNode;
    if (node) {
      let doubleClick = aEvent.button == 0 && aEvent.detail == 2;
      let middleClick = aEvent.button == 1 && aEvent.detail == 1;
      if (PlacesUtils.nodeIsURI(node) && (doubleClick || middleClick)) {
        
        this.openSelectedNode(aEvent);
      }
      else if (middleClick && PlacesUtils.nodeIsContainer(node)) {
        
        
        
        PlacesUIUtils.openContainerNodeInTabs(node, aEvent, this.view);
      }
    }
  },

  onKeyPress: function CT_onKeyPress(aEvent) {
    if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN)
      this.openSelectedNode(aEvent);
  }
};
