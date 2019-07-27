



const LAST_USED_ANNO = "bookmarkPropertiesDialog/folderLastUsed";
const MAX_FOLDER_ITEM_IN_MENU_LIST = 5;

var gEditItemOverlay = {
  _uri: null,
  _itemId: -1,
  _itemIds: [],
  _uris: [],
  _tags: [],
  _allTags: [],
  _multiEdit: false,
  _itemType: -1,
  _readOnly: false,
  _hiddenRows: [],
  _observersAdded: false,
  _staticFoldersListBuilt: false,
  _initialized: false,
  _titleOverride: "",

  
  
  _firstEditedField: "",

  get itemId() {
    return this._itemId;
  },

  get uri() {
    return this._uri;
  },

  get multiEdit() {
    return this._multiEdit;
  },

  


  _determineInfo: function EIO__determineInfo(aInfo) {
    
    if (aInfo && aInfo.hiddenRows)
      this._hiddenRows = aInfo.hiddenRows;
    else
      this._hiddenRows.splice(0, this._hiddenRows.length);
    
    this._readOnly = aInfo && aInfo.forceReadOnly;
    this._titleOverride = aInfo && aInfo.titleOverride ? aInfo.titleOverride
                                                       : "";
  },

  _showHideRows: function EIO__showHideRows() {
    var isBookmark = this._itemId != -1 &&
                     this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK;
    var isQuery = false;
    if (this._uri)
      isQuery = this._uri.schemeIs("place");

    this._element("nameRow").collapsed = this._hiddenRows.indexOf("name") != -1;
    this._element("folderRow").collapsed =
      this._hiddenRows.indexOf("folderPicker") != -1 || this._readOnly;
    this._element("tagsRow").collapsed = !this._uri ||
      this._hiddenRows.indexOf("tags") != -1 || isQuery;
    
    if (!this._element("tagsSelectorRow").collapsed &&
        this._element("tagsRow").collapsed)
      this.toggleTagsSelector();
    this._element("descriptionRow").collapsed =
      this._hiddenRows.indexOf("description") != -1 || this._readOnly;
    this._element("keywordRow").collapsed = !isBookmark || this._readOnly ||
      this._hiddenRows.indexOf("keyword") != -1 || isQuery;
    this._element("locationRow").collapsed = !(this._uri && !isQuery) ||
      this._hiddenRows.indexOf("location") != -1;
    this._element("loadInSidebarCheckbox").collapsed = !isBookmark || isQuery ||
      this._readOnly || this._hiddenRows.indexOf("loadInSidebar") != -1;
    this._element("feedLocationRow").collapsed = !this._isLivemark ||
      this._hiddenRows.indexOf("feedLocation") != -1;
    this._element("siteLocationRow").collapsed = !this._isLivemark ||
      this._hiddenRows.indexOf("siteLocation") != -1;
    this._element("selectionCount").hidden = !this._multiEdit;
  },

  















  initPanel: function EIO_initPanel(aFor, aInfo) {
    
    
    if (this._initialized)
      this.uninitPanel(false);

    var aItemIdList;
    if (Array.isArray(aFor)) {
      aItemIdList = aFor;
      aFor = aItemIdList[0];
    }
    else if (this._multiEdit) {
      this._multiEdit = false;
      this._tags = [];
      this._uris = [];
      this._allTags = [];
      this._itemIds = [];
      this._element("selectionCount").hidden = true;
    }

    this._folderMenuList = this._element("folderMenuList");
    this._folderTree = this._element("folderTree");

    this._determineInfo(aInfo);
    if (aFor instanceof Ci.nsIURI) {
      this._itemId = -1;
      this._uri = aFor;
      this._readOnly = true;
    }
    else {
      this._itemId = aFor;
      
      this._readOnly = this._readOnly || this._itemId == -1;

      var containerId = PlacesUtils.bookmarks.getFolderIdForItem(this._itemId);
      this._itemType = PlacesUtils.bookmarks.getItemType(this._itemId);
      if (this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
        this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);
        this._initTextField("keywordField",
                            PlacesUtils.bookmarks
                                       .getKeywordForBookmark(this._itemId));
        this._element("loadInSidebarCheckbox").checked =
          PlacesUtils.annotations.itemHasAnnotation(this._itemId,
                                                    PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO);
      }
      else {
        this._uri = null;
        this._isLivemark = false;
        PlacesUtils.livemarks.getLivemark({id: this._itemId })
          .then(aLivemark => {
            this._isLivemark = true;
            this._initTextField("feedLocationField", aLivemark.feedURI.spec, true);
            this._initTextField("siteLocationField", aLivemark.siteURI ? aLivemark.siteURI.spec : "", true);
            this._showHideRows();
          }, () => undefined);
      }

      
      this._initFolderMenuList(containerId);

      
      this._initTextField("descriptionField", 
                          PlacesUIUtils.getItemDescription(this._itemId));
    }

    if (this._itemId == -1 ||
        this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK) {
      this._isLivemark = false;

      this._initTextField("locationField", this._uri.spec);
      if (!aItemIdList) {
        var tags = PlacesUtils.tagging.getTagsForURI(this._uri).join(", ");
        this._initTextField("tagsField", tags, false);
      }
      else {
        this._multiEdit = true;
        this._allTags = [];
        this._itemIds = aItemIdList;
        for (var i = 0; i < aItemIdList.length; i++) {
          if (aItemIdList[i] instanceof Ci.nsIURI) {
            this._uris[i] = aItemIdList[i];
            this._itemIds[i] = -1;
          }
          else
            this._uris[i] = PlacesUtils.bookmarks.getBookmarkURI(this._itemIds[i]);
          this._tags[i] = PlacesUtils.tagging.getTagsForURI(this._uris[i]);
        }
        this._allTags = this._getCommonTags();
        this._initTextField("tagsField", this._allTags.join(", "), false);
        this._element("itemsCountText").value =
          PlacesUIUtils.getPluralString("detailsPane.itemsCountLabel",
                                        this._itemIds.length,
                                        [this._itemIds.length]);
      }

      
      this._rebuildTagsSelectorList();
    }

    
    this._initNamePicker();
    
    this._showHideRows();

    
    if (!this._observersAdded) {
      
      
      if (this._itemId != -1 || this._uri || this._multiEdit)
        PlacesUtils.bookmarks.addObserver(this, false);

      this._element("namePicker").addEventListener("blur", this);
      this._element("locationField").addEventListener("blur", this);
      this._element("tagsField").addEventListener("blur", this);
      this._element("keywordField").addEventListener("blur", this);
      this._element("descriptionField").addEventListener("blur", this);
      window.addEventListener("unload", this, false);
      this._observersAdded = true;
    }

    this._initialized = true;
  },

  






  _getCommonTags: function() {
    return this._tags[0].filter(
      function (aTag) this._tags.every(
        function (aTags) aTags.indexOf(aTag) != -1
      ), this
    );
  },

  _initTextField: function(aTextFieldId, aValue, aReadOnly) {
    var field = this._element(aTextFieldId);
    field.readOnly = aReadOnly !== undefined ? aReadOnly : this._readOnly;

    if (field.value != aValue) {
      field.value = aValue;

      
      var editor = field.editor;
      if (editor)
        editor.transactionManager.clear();
    }
  },

  







  _appendFolderItemToMenupopup:
  function EIO__appendFolderItemToMenuList(aMenupopup, aFolderId) {
    
    this._element("foldersSeparator").hidden = false;

    var folderMenuItem = document.createElement("menuitem");
    var folderTitle = PlacesUtils.bookmarks.getItemTitle(aFolderId)
    folderMenuItem.folderId = aFolderId;
    folderMenuItem.setAttribute("label", folderTitle);
    folderMenuItem.className = "menuitem-iconic folder-icon";
    aMenupopup.appendChild(folderMenuItem);
    return folderMenuItem;
  },

  _initFolderMenuList: function EIO__initFolderMenuList(aSelectedFolder) {
    
    var menupopup = this._folderMenuList.menupopup;
    while (menupopup.childNodes.length > 6)
      menupopup.removeChild(menupopup.lastChild);

    const bms = PlacesUtils.bookmarks;
    const annos = PlacesUtils.annotations;

    
    var unfiledItem = this._element("unfiledRootItem");
    if (!this._staticFoldersListBuilt) {
      unfiledItem.label = bms.getItemTitle(PlacesUtils.unfiledBookmarksFolderId);
      unfiledItem.folderId = PlacesUtils.unfiledBookmarksFolderId;
      var bmMenuItem = this._element("bmRootItem");
      bmMenuItem.label = bms.getItemTitle(PlacesUtils.bookmarksMenuFolderId);
      bmMenuItem.folderId = PlacesUtils.bookmarksMenuFolderId;
      var toolbarItem = this._element("toolbarFolderItem");
      toolbarItem.label = bms.getItemTitle(PlacesUtils.toolbarFolderId);
      toolbarItem.folderId = PlacesUtils.toolbarFolderId;
      this._staticFoldersListBuilt = true;
    }

    
    var folderIds = annos.getItemsWithAnnotation(LAST_USED_ANNO);

    







    this._recentFolders = [];
    for (var i = 0; i < folderIds.length; i++) {
      var lastUsed = annos.getItemAnnotation(folderIds[i], LAST_USED_ANNO);
      this._recentFolders.push({ folderId: folderIds[i], lastUsed: lastUsed });
    }
    this._recentFolders.sort(function(a, b) {
      if (b.lastUsed < a.lastUsed)
        return -1;
      if (b.lastUsed > a.lastUsed)
        return 1;
      return 0;
    });

    var numberOfItems = Math.min(MAX_FOLDER_ITEM_IN_MENU_LIST,
                                 this._recentFolders.length);
    for (var i = 0; i < numberOfItems; i++) {
      this._appendFolderItemToMenupopup(menupopup,
                                        this._recentFolders[i].folderId);
    }

    var defaultItem = this._getFolderMenuItem(aSelectedFolder);
    this._folderMenuList.selectedItem = defaultItem;

    
    this._folderMenuList.setAttribute("selectedIndex",
                                      this._folderMenuList.selectedIndex);

    
    this._element("foldersSeparator").hidden = (menupopup.childNodes.length <= 6);
    this._folderMenuList.disabled = this._readOnly;
  },

  QueryInterface: function EIO_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  _element: function EIO__element(aID) {
    return document.getElementById("editBMPanel_" + aID);
  },

  _getItemStaticTitle: function EIO__getItemStaticTitle() {
    if (this._titleOverride)
      return this._titleOverride;

    let title = "";
    if (this._itemId == -1) {
      title = PlacesUtils.history.getPageTitle(this._uri);
    }
    else {
      title = PlacesUtils.bookmarks.getItemTitle(this._itemId);
    }
    return title;
  },

  _initNamePicker: function EIO_initNamePicker() {
    var namePicker = this._element("namePicker");
    namePicker.value = this._getItemStaticTitle();
    namePicker.readOnly = this._readOnly;

    
    var editor = namePicker.editor;
    if (editor)
      editor.transactionManager.clear();
  },

  uninitPanel: function EIO_uninitPanel(aHideCollapsibleElements) {
    if (aHideCollapsibleElements) {
      
      var folderTreeRow = this._element("folderTreeRow");
      if (!folderTreeRow.collapsed)
        this.toggleFolderTreeVisibility();

      
      var tagsSelectorRow = this._element("tagsSelectorRow");
      if (!tagsSelectorRow.collapsed)
        this.toggleTagsSelector();
    }

    if (this._observersAdded) {
      if (this._itemId != -1 || this._uri || this._multiEdit)
        PlacesUtils.bookmarks.removeObserver(this);

      this._element("namePicker").removeEventListener("blur", this);
      this._element("locationField").removeEventListener("blur", this);
      this._element("tagsField").removeEventListener("blur", this);
      this._element("keywordField").removeEventListener("blur", this);
      this._element("descriptionField").removeEventListener("blur", this);

      this._observersAdded = false;
    }

    this._itemId = -1;
    this._uri = null;
    this._uris = [];
    this._tags = [];
    this._allTags = [];
    this._itemIds = [];
    this._multiEdit = false;
    this._firstEditedField = "";
    this._initialized = false;
    this._titleOverride = "";
    this._readOnly = false;
  },

  onTagsFieldBlur: function EIO_onTagsFieldBlur() {
    if (this._updateTags()) 
      this._mayUpdateFirstEditField("tagsField");
  },

  _updateTags: function EIO__updateTags() {
    if (this._multiEdit)
      return this._updateMultipleTagsForItems();
    return this._updateSingleTagForItem();
  },

  _updateSingleTagForItem: function EIO__updateSingleTagForItem() {
    var currentTags = PlacesUtils.tagging.getTagsForURI(this._uri);
    var tags = this._getTagsArrayFromTagField();
    if (tags.length > 0 || currentTags.length > 0) {
      var tagsToRemove = [];
      var tagsToAdd = [];
      var txns = []; 
      for (var i = 0; i < currentTags.length; i++) {
        if (tags.indexOf(currentTags[i]) == -1)
          tagsToRemove.push(currentTags[i]);
      }
      for (var i = 0; i < tags.length; i++) {
        if (currentTags.indexOf(tags[i]) == -1)
          tagsToAdd.push(tags[i]);
      }

      if (tagsToRemove.length > 0) {
        let untagTxn = new PlacesUntagURITransaction(this._uri, tagsToRemove);
        txns.push(untagTxn);
      }
      if (tagsToAdd.length > 0) {
        let tagTxn = new PlacesTagURITransaction(this._uri, tagsToAdd);
        txns.push(tagTxn);
      }

      if (txns.length > 0) {
        let aggregate = new PlacesAggregatedTransaction("Update tags", txns);
        PlacesUtils.transactionManager.doTransaction(aggregate);

        
        var tags = PlacesUtils.tagging.getTagsForURI(this._uri).join(", ");
        this._initTextField("tagsField", tags, false);
        return true;
      }
    }
    return false;
  },

   






  _mayUpdateFirstEditField: function EIO__mayUpdateFirstEditField(aNewField) {
    
    
    
    if (this._multiEdit || this._firstEditedField)
      return;

    this._firstEditedField = aNewField;

    
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    prefs.setCharPref("browser.bookmarks.editDialog.firstEditField", aNewField);
  },

  _updateMultipleTagsForItems: function EIO__updateMultipleTagsForItems() {
    var tags = this._getTagsArrayFromTagField();
    if (tags.length > 0 || this._allTags.length > 0) {
      var tagsToRemove = [];
      var tagsToAdd = [];
      var txns = []; 
      for (var i = 0; i < this._allTags.length; i++) {
        if (tags.indexOf(this._allTags[i]) == -1)
          tagsToRemove.push(this._allTags[i]);
      }
      for (var i = 0; i < this._tags.length; i++) {
        tagsToAdd[i] = [];
        for (var j = 0; j < tags.length; j++) {
          if (this._tags[i].indexOf(tags[j]) == -1)
            tagsToAdd[i].push(tags[j]);
        }
      }

      if (tagsToAdd.length > 0) {
        for (let i = 0; i < this._uris.length; i++) {
          if (tagsToAdd[i].length > 0) {
            let tagTxn = new PlacesTagURITransaction(this._uris[i],
                                                     tagsToAdd[i]);
            txns.push(tagTxn);
          }
        }
      }
      if (tagsToRemove.length > 0) {
        for (let i = 0; i < this._uris.length; i++) {
          let untagTxn = new PlacesUntagURITransaction(this._uris[i],
                                                       tagsToRemove);
          txns.push(untagTxn);
        }
      }

      if (txns.length > 0) {
        let aggregate = new PlacesAggregatedTransaction("Update tags", txns);
        PlacesUtils.transactionManager.doTransaction(aggregate);

        this._allTags = tags;
        this._tags = [];
        for (let i = 0; i < this._uris.length; i++) {
          this._tags[i] = PlacesUtils.tagging.getTagsForURI(this._uris[i]);
        }

        
        this._initTextField("tagsField", tags, false);
        return true;
      }
    }
    return false;
  },

  onNamePickerBlur: function EIO_onNamePickerBlur() {
    if (this._itemId == -1)
      return;

    var namePicker = this._element("namePicker")

    
    var newTitle = namePicker.value;
    if (!newTitle &&
        PlacesUtils.bookmarks.getFolderIdForItem(this._itemId) == PlacesUtils.tagsFolderId) {
      
      this._initNamePicker();
    }
    else if (this._getItemStaticTitle() != newTitle) {
      this._mayUpdateFirstEditField("namePicker");
      let txn = new PlacesEditItemTitleTransaction(this._itemId, newTitle);
      PlacesUtils.transactionManager.doTransaction(txn);
    }
  },

  onDescriptionFieldBlur: function EIO_onDescriptionFieldBlur() {
    var description = this._element("descriptionField").value;
    if (description != PlacesUIUtils.getItemDescription(this._itemId)) {
      var annoObj = { name   : PlacesUIUtils.DESCRIPTION_ANNO,
                      type   : Ci.nsIAnnotationService.TYPE_STRING,
                      flags  : 0,
                      value  : description,
                      expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
      var txn = new PlacesSetItemAnnotationTransaction(this._itemId, annoObj);
      PlacesUtils.transactionManager.doTransaction(txn);
    }
  },

  onLocationFieldBlur: function EIO_onLocationFieldBlur() {
    var uri;
    try {
      uri = PlacesUIUtils.createFixedURI(this._element("locationField").value);
    }
    catch(ex) { return; }

    if (!this._uri.equals(uri)) {
      var txn = new PlacesEditBookmarkURITransaction(this._itemId, uri);
      PlacesUtils.transactionManager.doTransaction(txn);
      this._uri = uri;
    }
  },

  onKeywordFieldBlur: function EIO_onKeywordFieldBlur() {
    var keyword = this._element("keywordField").value;
    if (keyword != PlacesUtils.bookmarks.getKeywordForBookmark(this._itemId)) {
      var txn = new PlacesEditBookmarkKeywordTransaction(this._itemId, keyword);
      PlacesUtils.transactionManager.doTransaction(txn);
    }
  },

  onLoadInSidebarCheckboxCommand:
  function EIO_onLoadInSidebarCheckboxCommand() {
    let annoObj = { name : PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO };
    if (this._element("loadInSidebarCheckbox").checked)
      annoObj.value = true;
    let txn = new PlacesSetItemAnnotationTransaction(this._itemId, annoObj);
    PlacesUtils.transactionManager.doTransaction(txn);
  },

  toggleFolderTreeVisibility: function EIO_toggleFolderTreeVisibility() {
    var expander = this._element("foldersExpander");
    var folderTreeRow = this._element("folderTreeRow");
    if (!folderTreeRow.collapsed) {
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
      folderTreeRow.collapsed = true;
      this._element("chooseFolderSeparator").hidden =
        this._element("chooseFolderMenuItem").hidden = false;
    }
    else {
      expander.className = "expander-up"
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextup"));
      folderTreeRow.collapsed = false;

      
      
      
      const FOLDER_TREE_PLACE_URI =
        "place:excludeItems=1&excludeQueries=1&excludeReadOnlyFolders=1&folder=" +
        PlacesUIUtils.allBookmarksFolderId;
      this._folderTree.place = FOLDER_TREE_PLACE_URI;

      this._element("chooseFolderSeparator").hidden =
        this._element("chooseFolderMenuItem").hidden = true;
      var currentFolder = this._getFolderIdFromMenuList();
      this._folderTree.selectItems([currentFolder]);
      this._folderTree.focus();
    }
  },

  _getFolderIdFromMenuList:
  function EIO__getFolderIdFromMenuList() {
    var selectedItem = this._folderMenuList.selectedItem;
    NS_ASSERT("folderId" in selectedItem,
              "Invalid menuitem in the folders-menulist");
    return selectedItem.folderId;
  },

  







  _getFolderMenuItem:
  function EIO__getFolderMenuItem(aFolderId) {
    var menupopup = this._folderMenuList.menupopup;

    for (let i = 0; i < menupopup.childNodes.length; i++) {
      if ("folderId" in menupopup.childNodes[i] &&
          menupopup.childNodes[i].folderId == aFolderId)
        return menupopup.childNodes[i];
    }

    
    if (menupopup.childNodes.length == 4 + MAX_FOLDER_ITEM_IN_MENU_LIST)
      menupopup.removeChild(menupopup.lastChild);

    return this._appendFolderItemToMenupopup(menupopup, aFolderId);
  },

  onFolderMenuListCommand: function EIO_onFolderMenuListCommand(aEvent) {
    
    this._folderMenuList.setAttribute("selectedIndex",
                                      this._folderMenuList.selectedIndex);

    if (aEvent.target.id == "editBMPanel_chooseFolderMenuItem") {
      
      
      var container = PlacesUtils.bookmarks.getFolderIdForItem(this._itemId);
      var item = this._getFolderMenuItem(container);
      this._folderMenuList.selectedItem = item;
      
      
      setTimeout(function(self) self.toggleFolderTreeVisibility(), 100, this);
      return;
    }

    
    var container = this._getFolderIdFromMenuList();
    if (PlacesUtils.bookmarks.getFolderIdForItem(this._itemId) != container) {
      var txn = new PlacesMoveItemTransaction(this._itemId, 
                                              container, 
                                              PlacesUtils.bookmarks.DEFAULT_INDEX);
      PlacesUtils.transactionManager.doTransaction(txn);

      
      
      if (container != PlacesUtils.unfiledBookmarksFolderId &&
          container != PlacesUtils.toolbarFolderId &&
          container != PlacesUtils.bookmarksMenuFolderId)
        this._markFolderAsRecentlyUsed(container);
    }

    
    var folderTreeRow = this._element("folderTreeRow");
    if (!folderTreeRow.collapsed) {
      var selectedNode = this._folderTree.selectedNode;
      if (!selectedNode ||
          PlacesUtils.getConcreteItemId(selectedNode) != container)
        this._folderTree.selectItems([container]);
    }
  },

  onFolderTreeSelect: function EIO_onFolderTreeSelect() {
    var selectedNode = this._folderTree.selectedNode;

    
    this._element("newFolderButton")
        .disabled = !this._folderTree.insertionPoint || !selectedNode;

    if (!selectedNode)
      return;

    var folderId = PlacesUtils.getConcreteItemId(selectedNode);
    if (this._getFolderIdFromMenuList() == folderId)
      return;

    var folderItem = this._getFolderMenuItem(folderId);
    this._folderMenuList.selectedItem = folderItem;
    folderItem.doCommand();
  },

  _markFolderAsRecentlyUsed:
  function EIO__markFolderAsRecentlyUsed(aFolderId) {
    var txns = [];

    
    var anno = this._getLastUsedAnnotationObject(false);
    while (this._recentFolders.length > MAX_FOLDER_ITEM_IN_MENU_LIST) {
      var folderId = this._recentFolders.pop().folderId;
      let annoTxn = new PlacesSetItemAnnotationTransaction(folderId, anno);
      txns.push(annoTxn);
    }

    
    anno = this._getLastUsedAnnotationObject(true);
    let annoTxn = new PlacesSetItemAnnotationTransaction(aFolderId, anno);
    txns.push(annoTxn);

    let aggregate = new PlacesAggregatedTransaction("Update last used folders", txns);
    PlacesUtils.transactionManager.doTransaction(aggregate);
  },

  








  _getLastUsedAnnotationObject:
  function EIO__getLastUsedAnnotationObject(aLastUsed) {
    var anno = { name: LAST_USED_ANNO,
                 type: Ci.nsIAnnotationService.TYPE_INT32,
                 flags: 0,
                 value: aLastUsed ? new Date().getTime() : null,
                 expires: Ci.nsIAnnotationService.EXPIRE_NEVER };

    return anno;
  },

  _rebuildTagsSelectorList: function EIO__rebuildTagsSelectorList() {
    var tagsSelector = this._element("tagsSelector");
    var tagsSelectorRow = this._element("tagsSelectorRow");
    if (tagsSelectorRow.collapsed)
      return;

    
    let firstIndex = tagsSelector.getIndexOfFirstVisibleRow();
    let selectedIndex = tagsSelector.selectedIndex;
    let selectedTag = selectedIndex >= 0 ? tagsSelector.selectedItem.label
                                         : null;

    while (tagsSelector.hasChildNodes())
      tagsSelector.removeChild(tagsSelector.lastChild);

    var tagsInField = this._getTagsArrayFromTagField();
    var allTags = PlacesUtils.tagging.allTags;
    for (var i = 0; i < allTags.length; i++) {
      var tag = allTags[i];
      var elt = document.createElement("listitem");
      elt.setAttribute("type", "checkbox");
      elt.setAttribute("label", tag);
      if (tagsInField.indexOf(tag) != -1)
        elt.setAttribute("checked", "true");
      tagsSelector.appendChild(elt);
      if (selectedTag === tag)
        selectedIndex = tagsSelector.getIndexOfItem(elt);
    }

    
    
    
    firstIndex =
      Math.min(firstIndex,
               tagsSelector.itemCount - tagsSelector.getNumberOfVisibleRows());
    tagsSelector.scrollToIndex(firstIndex);
    if (selectedIndex >= 0 && tagsSelector.itemCount > 0) {
      selectedIndex = Math.min(selectedIndex, tagsSelector.itemCount - 1);
      tagsSelector.selectedIndex = selectedIndex;
      tagsSelector.ensureIndexIsVisible(selectedIndex);
    }
  },

  toggleTagsSelector: function EIO_toggleTagsSelector() {
    var tagsSelector = this._element("tagsSelector");
    var tagsSelectorRow = this._element("tagsSelectorRow");
    var expander = this._element("tagsSelectorExpander");
    if (tagsSelectorRow.collapsed) {
      expander.className = "expander-up";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextup"));
      tagsSelectorRow.collapsed = false;
      this._rebuildTagsSelectorList();

      
      tagsSelector.addEventListener("CheckboxStateChange", this, false);
    }
    else {
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
      tagsSelectorRow.collapsed = true;
    }
  },

  




  _getTagsArrayFromTagField: function EIO__getTagsArrayFromTagField() {
    let tags = this._element("tagsField").value;
    return tags.trim()
               .split(/\s*,\s*/) 
               .filter(function (tag) tag.length > 0); 
  },

  newFolder: function EIO_newFolder() {
    var ip = this._folderTree.insertionPoint;

    
    if (!ip || ip.itemId == PlacesUIUtils.allBookmarksFolderId) {
        ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId,
                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                Ci.nsITreeView.DROP_ON);
    }

    
    var defaultLabel = this._element("newFolderButton").label;
    var txn = new PlacesCreateFolderTransaction(defaultLabel, ip.itemId, ip.index);
    PlacesUtils.transactionManager.doTransaction(txn);
    this._folderTree.focus();
    this._folderTree.selectItems([ip.itemId]);
    PlacesUtils.asContainer(this._folderTree.selectedNode).containerOpen = true;
    this._folderTree.selectItems([this._lastNewItem]);
    this._folderTree.startEditing(this._folderTree.view.selection.currentIndex,
                                  this._folderTree.columns.getFirstColumn());
  },

  
  handleEvent: function EIO_nsIDOMEventListener(aEvent) {
    switch (aEvent.type) {
    case "CheckboxStateChange":
      
      let tags = this._getTagsArrayFromTagField();
      let tagCheckbox = aEvent.target;

      let curTagIndex = tags.indexOf(tagCheckbox.label);

      let tagsSelector = this._element("tagsSelector");
      tagsSelector.selectedItem = tagCheckbox;

      if (tagCheckbox.checked) {
        if (curTagIndex == -1)
          tags.push(tagCheckbox.label);
      }
      else {
        if (curTagIndex != -1)
          tags.splice(curTagIndex, 1);
      }
      this._element("tagsField").value = tags.join(", ");
      this._updateTags();
      break;
    case "blur":
      let replaceFn = (str, firstLetter) => firstLetter.toUpperCase();
      let nodeName = aEvent.target.id.replace(/editBMPanel_(\w)/, replaceFn);
      this["on" + nodeName + "Blur"]();
      break;
    case "unload":
      this.uninitPanel(false);
      break;
    }
  },

  
  onItemChanged: function EIO_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aValue,
                                            aLastModified, aItemType) {
    if (aProperty == "tags") {
      
      
      
      
      let shouldUpdateTagsField = this._itemId == aItemId;
      if (this._itemId == -1 || this._multiEdit) {
        
        let changedURI = PlacesUtils.bookmarks.getBookmarkURI(aItemId);
        let uris = this._multiEdit ? this._uris : [this._uri];
        uris.forEach(function (aURI, aIndex) {
          if (aURI.equals(changedURI)) {
            shouldUpdateTagsField = true;
            if (this._multiEdit) {
              this._tags[aIndex] = PlacesUtils.tagging.getTagsForURI(this._uris[aIndex]);
            }
          }
        }, this);
      }

      if (shouldUpdateTagsField) {
        if (this._multiEdit) {
          this._allTags = this._getCommonTags();
          this._initTextField("tagsField", this._allTags.join(", "), false);
        }
        else {
          let tags = PlacesUtils.tagging.getTagsForURI(this._uri).join(", ");
          this._initTextField("tagsField", tags, false);
        }
      }

      
      this._rebuildTagsSelectorList();
      return;
    }

    if (this._itemId != aItemId) {
      if (aProperty == "title") {
        
        
        
        var menupopup = this._folderMenuList.menupopup;
        for (let i = 0; i < menupopup.childNodes.length; i++) {
          if ("folderId" in menupopup.childNodes[i] &&
              menupopup.childNodes[i].folderId == aItemId) {
            menupopup.childNodes[i].label = aValue;
            break;
          }
        }
      }

      return;
    }

    switch (aProperty) {
    case "title":
      var namePicker = this._element("namePicker");
      if (namePicker.value != aValue) {
        namePicker.value = aValue;
        
        namePicker.editor.transactionManager.clear();
      }
      break;
    case "uri":
      var locationField = this._element("locationField");
      if (locationField.value != aValue) {
        this._uri = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService).
                    newURI(aValue, null, null);
        this._initTextField("locationField", this._uri.spec);
        this._initNamePicker();
        this._initTextField("tagsField",
                             PlacesUtils.tagging
                                        .getTagsForURI(this._uri).join(", "),
                            false);
        this._rebuildTagsSelectorList();
      }
      break;
    case "keyword":
      this._initTextField("keywordField",
                          PlacesUtils.bookmarks
                                     .getKeywordForBookmark(this._itemId));
      break;
    case PlacesUIUtils.DESCRIPTION_ANNO:
      this._initTextField("descriptionField",
                          PlacesUIUtils.getItemDescription(this._itemId));
      break;
    case PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO:
      this._element("loadInSidebarCheckbox").checked =
        PlacesUtils.annotations.itemHasAnnotation(this._itemId,
                                                  PlacesUIUtils.LOAD_IN_SIDEBAR_ANNO);
      break;
    case PlacesUtils.LMANNO_FEEDURI:
      let feedURISpec =
        PlacesUtils.annotations.getItemAnnotation(this._itemId,
                                                  PlacesUtils.LMANNO_FEEDURI);
      this._initTextField("feedLocationField", feedURISpec, true);
      break;
    case PlacesUtils.LMANNO_SITEURI:
      let siteURISpec = "";
      try {
        siteURISpec =
          PlacesUtils.annotations.getItemAnnotation(this._itemId,
                                                    PlacesUtils.LMANNO_SITEURI);
      } catch (ex) {}
      this._initTextField("siteLocationField", siteURISpec, true);
      break;
    }
  },

  onItemMoved: function EIO_onItemMoved(aItemId, aOldParent, aOldIndex,
                                        aNewParent, aNewIndex, aItemType) {
    if (aItemId != this._itemId ||
        aNewParent == this._getFolderIdFromMenuList())
      return;

    var folderItem = this._getFolderMenuItem(aNewParent);

    
    
    this._folderMenuList.selectedItem = folderItem;
  },

  onItemAdded: function EIO_onItemAdded(aItemId, aParentId, aIndex, aItemType,
                                        aURI) {
    this._lastNewItem = aItemId;
  },

  onItemRemoved: function() { },
  onBeginUpdateBatch: function() { },
  onEndUpdateBatch: function() { },
  onItemVisited: function() { },
};
