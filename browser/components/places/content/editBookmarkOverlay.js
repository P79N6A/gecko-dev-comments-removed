




































const LAST_USED_ANNO = "bookmarkPropertiesDialog/lastUsed";
const MAX_FOLDER_ITEM_IN_MENU_LIST = 5;

var gAddBookmarksPanel = {
  


  __mss: null,
  get _mss() {
    if (!this.__mss)
      this.__mss = Cc["@mozilla.org/microsummary/service;1"].
                  getService(Ci.nsIMicrosummaryService);
    return this.__mss;
  },

  _uri: null,
  _itemId: -1,
  _itemType: -1,
  _microsummaries: null,
  _doneCallback: null,
  _currentTags: [],
  _hiddenRows: [],

  


  _determineInfo: function ABP__determineInfo(aInfo) {
    const bms = PlacesUtils.bookmarks;
    this._itemType = bms.getItemType(this._itemId);
    if (this._itemType == Ci.nsINavBookmarksService.TYPE_BOOKMARK)
      this._currentTags = PlacesUtils.tagging.getTagsForURI(this._uri);
    else
      this._currentTags.splice(0);

    
    if (aInfo && aInfo.hiddenRows)
      this._hiddenRows = aInfo.hiddenRows;
    else
      this._hiddenRows.splice(0);
  },

  _showHideRows: function EBP__showHideRows() {
    this._element("nameRow").hidden = this._hiddenRows.indexOf("name") != -1;
    this._element("folderRow").hidden =
      this._hiddenRows.indexOf("folderPicker") != -1;
    this._element("tagsRow").hidden = this._hiddenRows.indexOf("tags") != -1 ||
      this._itemType != Ci.nsINavBookmarksService.TYPE_BOOKMARK;
    this._element("descriptionRow").hidden =
      this._hiddenRows.indexOf("description") != -1;
  },

  


  initPanel: function ABP_initPanel(aItemId, aTm, aDoneCallback, aInfo) {
    this._folderMenuList = this._element("folderMenuList");
    this._folderTree = this._element("folderTree");
    this._tm = aTm;
    this._itemId = aItemId;
    this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);
    this._doneCallback = aDoneCallback;
    this._determineInfo(aInfo);

    
    this._initFolderMenuList();
    
    
    this._initNamePicker();

    
    this._element("tagsField").value = this._currentTags.join(", ");

    
    this._element("descriptionField").value =
      PlacesUtils.getItemDescription(this._itemId);

    this._showHideRows();
  },

  







  _appendFolderItemToMenupopup:
  function BPP__appendFolderItemToMenuList(aMenupopup, aFolderId) {
    
    this._element("foldersSeparator").hidden = false;

    var folderMenuItem = document.createElement("menuitem");
    var folderTitle = PlacesUtils.bookmarks.getItemTitle(aFolderId)
    folderMenuItem.folderId = aFolderId;
    folderMenuItem.setAttribute("label", folderTitle);
    folderMenuItem.className = "menuitem-iconic folder-icon";
    aMenupopup.appendChild(folderMenuItem);
    return folderMenuItem;
  },

  _initFolderMenuList: function BPP__initFolderMenuList() {
    
    var menupopup = this._folderMenuList.menupopup;
    while (menupopup.childNodes.length > 4)
      menupopup.removeChild(menupopup.lastChild);

    var container = PlacesUtils.bookmarks.getFolderIdForItem(this._itemId);

    
    this._element("placesRootItem").hidden = container != PlacesUtils.placesRootId;

    
    var annos = PlacesUtils.annotations;
    var folderIds = annos.getItemsWithAnnotation(LAST_USED_ANNO, { });

    







    var folders = [];
    for (var i=0; i < folderIds.length; i++) {
      var lastUsed = annos.getItemAnnotation(folderIds[i], LAST_USED_ANNO);
      folders.push({ folderId: folderIds[i], lastUsed: lastUsed });
    }
    folders.sort(function(a, b) {
      if (b.lastUsed < a.lastUsed)
        return -1;
      if (b.lastUsed > a.lastUsed)
        return 1;
      return 0;
    });

    var numberOfItems = Math.min(MAX_FOLDER_ITEM_IN_MENU_LIST, folders.length);
    for (i=0; i < numberOfItems; i++) {
      this._appendFolderItemToMenupopup(menupopup, folders[i].folderId);
    }

    var defaultItem = this._getFolderMenuItem(container, true);
    this._folderMenuList.selectedItem = defaultItem;

    
    this._element("foldersSeparator").hidden = (menupopup.childNodes.length <= 4);
  },

  QueryInterface: function BPP_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIMicrosummaryObserver) ||
        aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.eqauls(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  _element: function BPP__element(aID) {
    return document.getElementById("editBMPanel_" + aID);
  },

  _createMicrosummaryMenuItem:
  function BPP__createMicrosummaryMenuItem(aMicrosummary) {
    var menuItem = document.createElement("menuitem");

    
    
    
    menuItem.microsummary = aMicrosummary;

    
    
    
    
    
    
    
    
    
    if (aMicrosummary.content)
      menuItem.setAttribute("label", aMicrosummary.content);
    else {
      menuItem.setAttribute("label", aMicrosummary.generator.name ||
                                     aMicrosummary.generator.uri.spec);
      aMicrosummary.update();
    }

    return menuItem;
  },

  _initNamePicker: function ABP_initNamePicker() {
    var userEnteredNameField = this._element("userEnteredName");
    var namePicker = this._element("namePicker");
    var droppable = false;

    userEnteredNameField.label =
      PlacesUtils.bookmarks.getItemTitle(this._itemId);

    
    var menupopup = namePicker.menupopup;
    while (menupopup.childNodes.length > 2)
      menupopup.removeChild(menupopup.lastChild);

    var itemToSelect = userEnteredNameField;
    try {
      this._microsummaries = this._mss.getMicrosummaries(this._uri, -1);
    }
    catch(ex) {
      
      
      
      
      
      
      this._microsummaries = null;
    }
    if (this._microsummaries) {
      var enumerator = this._microsummaries.Enumerate();

      if (enumerator.hasMoreElements()) {
        
        droppable = true;
        while (enumerator.hasMoreElements()) {
          var microsummary = enumerator.getNext()
                                       .QueryInterface(Ci.nsIMicrosummary);
          var menuItem = this._createMicrosummaryMenuItem(microsummary);
          menupopup.appendChild(menuItem);
        }
      }

      this._microsummaries.addObserver(this);
    }

    if (namePicker.selectedItem == itemToSelect)
      namePicker.value = itemToSelect.label;
    else
      namePicker.selectedItem = itemToSelect;

    namePicker.setAttribute("droppable", droppable);
  },

  
  onContentLoaded: function ABP_onContentLoaded(aMicrosummary) {
    var namePicker = this._element("namePicker");
    var childNodes = namePicker.menupopup.childNodes;

    
    for (var i = 2; i < childNodes.length; i++) {
      if (childNodes[i].microsummary == aMicrosummary) {
        var newLabel = aMicrosummary.content;
        
        
        
        
        
        
        
        if (namePicker.selectedItem == childNodes[i])
          namePicker.value = newLabel;

        childNodes[i].label = newLabel;
        return;
      }
    }
  },

  onElementAppended: function BPP_onElementAppended(aMicrosummary) {
    var namePicker = this._element("namePicker");
    namePicker.menupopup
              .appendChild(this._createMicrosummaryMenuItem(aMicrosummary));

    
    namePicker.setAttribute("droppable", "true");
  },

  uninitPanel: function ABP_uninitPanel() {
    if (this._microsummaries)
      this._microsummaries.removeObserver(this);

    
    if (!this._folderTree.collapsed)
      this.toggleFolderTreeVisibility();

    
    var tagsSelector = this._element("tagsSelector");
    if (!tagsSelector.collapsed)
      tagsSelector.collapsed = true;
  },

  saveItem: function ABP_saveItem() {
    var container = this._getFolderIdFromMenuList();
    const bms = PlacesUtils.bookmarks;
    const ptm = PlacesUtils.ptm;
    var txns = [];

    
    if (bms.getFolderIdForItem(this._itemId) != container)
      txns.push(ptm.moveItem(this._itemId, container, -1));

    
    var newTitle = this._element("userEnteredName").label;
    if (bms.getItemTitle(this._itemId) != newTitle)
      txns.push(ptm.editItemTitle(this._itemId, newTitle));

    
    var newDescription = this._element("descriptionField").value;
    if (newDescription != PlacesUtils.getItemDescription(this._itemId))
      txns.push(ptm.editItemDescription(this._itemId, newDescription));

    
    var tags = this._getTagsArrayFromTagField();
    if (tags.length > 0 || this._currentTags.length > 0) {
      var tagsToRemove = [];
      var tagsToAdd = [];
      var t;
      for each (t in this._currentTags) {
        if (tags.indexOf(t) == -1)
          tagsToRemove.push(t);
      }
      for each (t in tags) {
        if (this._currentTags.indexOf(t) == -1)
          tagsToAdd.push(t);
      }

      if (tagsToAdd.length > 0)
        PlacesUtils.tagging.tagURI(this._uri, tagsToAdd);
      if (tagsToRemove.length > 0)
        PlacesUtils.tagging.untagURI(this._uri, tagsToRemove);
    }

    if (txns.length > 0) {
      
      
      if (container != PlacesUtils.placesRootId)
        this._markFolderAsRecentlyUsed(container);
    }

    if (txns.length > 0)
      ptm.commitTransaction(ptm.aggregateTransactions("Edit Item", txns));
  },

  onNamePickerInput: function ABP_onNamePickerInput() {
    this._element("userEnteredName").label = this._element("namePicker").value;
  },

  toggleFolderTreeVisibility: function ABP_toggleFolderTreeVisibility() {
    var expander = this._element("foldersExpander");
    if (!this._folderTree.collapsed) {
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
      this._folderTree.collapsed = true;
    }
    else {
      expander.className = "expander-up"
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextup"));
      if (!this._folderTree.treeBoxObject.view.isContainerOpen(0))
        this._folderTree.treeBoxObject.view.toggleOpenState(0);
      this._folderTree.selectFolders([this._getFolderIdFromMenuList()]);
      this._folderTree.collapsed = false;
      this._folderTree.focus();
    }
  },

  _getFolderIdFromMenuList:
  function BPP__getFolderIdFromMenuList() {
    var selectedItem = this._folderMenuList.selectedItem
    switch (selectedItem.id) {
      case "editBMPanel_placesRootItem":
        return PlacesUtils.placesRootId;
      case "editBMPanel_bmRootItem":
        return PlacesUtils.bookmarksRootId;
      case "editBMPanel_toolbarFolderItem":
        return PlacesUtils.toolbarFolderId;
    }

    NS_ASSERT("folderId" in selectedItem,
              "Invalid menuitem in the folders-menulist");
    return selectedItem.folderId;
  },

  











  _getFolderMenuItem:
  function BPP__getFolderMenuItem(aFolderId, aCheckStaticFolderItems) {
    var menupopup = this._folderMenuList.menupopup;

    
    for (var i=4;  i < menupopup.childNodes.length; i++) {
      if (menupopup.childNodes[i].folderId == aFolderId)
        return menupopup.childNodes[i];
    }

    if (aCheckStaticFolderItems) {
      if (aFolderId == PlacesUtils.placesRootId)
        return this._element("placesRootItem");
      if (aFolderId == PlacesUtils.bookmarksRootId)
        return this._element("bmRootItem")
      if (aFolderId == PlacesUtils.toolbarFolderId)
        return this._element("toolbarFolderItem")
    }

    
    if (menupopup.childNodes.length == 4 + MAX_FOLDER_ITEM_IN_MENU_LIST)
      menupopup.removeChild(menupopup.lastChild);

    return this._appendFolderItemToMenupopup(menupopup, aFolderId);
  },

  onMenuListFolderSelect: function BPP_onMenuListFolderSelect(aEvent) {
    if (this._folderTree.hidden)
      return;

    this._folderTree.selectFolders([this._getFolderIdFromMenuList()]);
  },

  onFolderTreeSelect: function BPP_onFolderTreeSelect() {
    var selectedNode = this._folderTree.selectedNode;
    if (!selectedNode)
      return;

    var folderId = selectedNode.itemId;
    
    
    var oldSelectedItem = this._folderMenuList.selectedItem;
    if ((oldSelectedItem.id == "editBMPanel_toolbarFolderItem" &&
         folderId == PlacesUtils.bookmarks.toolbarFolder) ||
        (oldSelectedItem.id == "editBMPanel_bmRootItem" &&
         folderId == PlacesUtils.bookmarks.bookmarksRoot))
      return;

    var folderItem = this._getFolderMenuItem(folderId, false);
    this._folderMenuList.selectedItem = folderItem;
  },

  _markFolderAsRecentlyUsed:
  function ABP__markFolderAsRecentlyUsed(aFolderId) {
    
    
    PlacesUtils.annotations
               .setItemAnnotation(aFolderId, LAST_USED_ANNO,
                                  new Date().getTime(), 0,
                                  Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  accept: function ABP_accept() {
    this.saveItem();
    if (typeof(this._doneCallback) == "function")
      this._doneCallback();
  },

  deleteAndClose: function ABP_deleteAndClose() {
    
    if (this._itemId != -1)
      PlacesUtils.bookmarks.removeItem(this._itemId);

    
    PlacesUtils.tagging.untagURI(this._uri, null);

    if (typeof(this._doneCallback) == "function")
      this._doneCallback();
  },

  toggleTagsSelector: function ABP_toggleTagsSelector() {
    var tagsSelector = this._element("tagsSelector");
    var expander = this._element("tagsSelectorExpander");
    if (tagsSelector.collapsed) {
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));

      
      while (tagsSelector.hasChildNodes())
        tagsSelector.removeChild(tagsSelector.lastChild);

      var tagsInField = this._getTagsArrayFromTagField();
      var allTags = PlacesUtils.tagging.allTags;
      for each (var tag in allTags) {
        var elt = document.createElement("listitem");
        elt.setAttribute("type", "checkbox");
        elt.setAttribute("label", tag);
        if (tagsInField.indexOf(tag) != -1)
          elt.setAttribute("checked", "true");

        tagsSelector.appendChild(elt);
      }

      
      tagsSelector.addEventListener("CheckboxStateChange", this, false);
    }
    else {
      expander.className = "expander-down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
    }

    tagsSelector.collapsed = !tagsSelector.collapsed;
  },

  _getTagsArrayFromTagField: function() {
    
    var tags = this._element("tagsField").value.split(",");
    for (var i=0; i < tags.length; i++) {
      
      tags[i] = tags[i].replace(/^\s+/, "").replace(/\s+$/, "");

      
      if (tags[i] == "") {
        tags.splice(i, 1);
        i--;
      }
    }
    return tags;
  },

  
  handleEvent: function ABP_nsIDOMEventListener(aEvent) {
    if (aEvent.type == "CheckboxStateChange") {
      
      var tags = this._getTagsArrayFromTagField();

      if (aEvent.target.checked)
        tags.push(aEvent.target.label);
      else {
        var indexOfItem = tags.indexOf(aEvent.target.label);
        if (indexOfItem != -1)
          tags.splice(indexOfItem, 1);
      }
      this._element("tagsField").value = tags.join(", ");
    }
  }
};
