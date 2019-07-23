


























































































const LAST_USED_ANNO = "bookmarkPropertiesDialog/lastUsed";


const MAX_FOLDER_ITEM_IN_MENU_LIST = 5;

const BOOKMARK_ITEM = 0;
const BOOKMARK_FOLDER = 1;
const LIVEMARK_CONTAINER = 2;

const ACTION_EDIT = 0;
const ACTION_ADD = 1;

var BookmarkPropertiesPanel = {

  
  __strings: null,
  get _strings() {
    if (!this.__strings) {
      this.__strings = document.getElementById("stringBundle");
    }
    return this.__strings;
  },

  


  __mss: null,
  get _mss() {
    if (!this.__mss)
      this.__mss = Cc["@mozilla.org/microsummary/service;1"].
                  getService(Ci.nsIMicrosummaryService);
    return this.__mss;
  },

  _action: null,
  _itemType: null,
  _folderId: null,
  _bookmarkId: -1,
  _bookmarkURI: null,
  _loadBookmarkInSidebar: false,
  _itemTitle: "",
  _itemDescription: "",
  _microsummaries: null,
  _URIList: null,
  _postData: null,

  
  
  
  
  
  
  _folderTreeHeight: null,

  



  _getAcceptLabel: function BPP__getAcceptLabel() {
    if (this._action == ACTION_ADD) {
      if (this._URIList)
        return this._strings.getString("dialogAcceptLabelAddMulti");

      return this._strings.getString("dialogAcceptLabelAddItem");
    }
    return this._strings.getString("dialogAcceptLabelEdit");
  },

  



  _getDialogTitle: function BPP__getDialogTitle() {
    if (this._action == ACTION_ADD) {
      if (this._itemType == BOOKMARK_ITEM)
        return this._strings.getString("dialogTitleAddBookmark");
      if (this._itemType == LIVEMARK_CONTAINER)
        return this._strings.getString("dialogTitleAddLivemark");

      
      NS_ASSERT(this._itemType == BOOKMARK_FOLDER, "bogus item type");
      if (this._URIList)
        return this._strings.getString("dialogTitleAddMulti");

      return this._strings.getString("dialogTitleAddFolder");
    }
    if (this._action == ACTION_EDIT) {
      return this._strings
                 .getFormattedString("dialogTitleEdit", [this._itemTitle]);
    }
    return "";
  },

  


  _determineItemInfo: function BPP__determineItemInfo() {
    var dialogInfo = window.arguments[0];
    NS_ASSERT("action" in dialogInfo, "missing action property");
    var action = dialogInfo.action;

    if (action == "add") {
      NS_ASSERT("type" in dialogInfo, "missing type property for add action");

      if ("title" in dialogInfo)
        this._itemTitle = dialogInfo.title;
      if ("defaultInsertionPoint" in dialogInfo)
        this._defaultInsertionPoint = dialogInfo.defaultInsertionPoint;
      else {
        
        this._defaultInsertionPoint =
          new InsertionPoint(PlacesUtils.bookmarks.bookmarksRoot, -1);
      }

      switch(dialogInfo.type) {
        case "bookmark":
          this._action = ACTION_ADD;
          this._itemType = BOOKMARK_ITEM;
          if ("uri" in dialogInfo) {
            NS_ASSERT(dialogInfo.uri instanceof Ci.nsIURI,
                      "uri property should be a uri object");
            this._bookmarkURI = dialogInfo.uri;
          }
          if (typeof(this._itemTitle) != "string") {
            if (this._bookmarkURI) {
              this._itemTitle =
                this._getURITitleFromHistory(this._bookmarkURI);
              if (!this._itemTitle)
                this._itemTitle = this._bookmarkURI.spec;
            }
            else
              this._itemTitle = this._strings.getString("newBookmarkDefault");
          }

          if ("loadBookmarkInSidebar" in dialogInfo)
            this._loadBookmarkInSidebar = dialogInfo.loadBookmarkInSidebar;

          if ("keyword" in dialogInfo) {
            this._bookmarkKeyword = dialogInfo.keyword;
            if ("postData" in dialogInfo)
              this._postData = dialogInfo.postData;
          }

          break;
        case "folder":
          this._action = ACTION_ADD;
          this._itemType = BOOKMARK_FOLDER;
          if (!this._itemTitle) {
            if ("URIList" in dialogInfo) {
              this._itemTitle =
                this._strings.getString("bookmarkAllTabsDefault");
              this._URIList = dialogInfo.URIList;
            }
            else
              this._itemTitle = this._strings.getString("newFolderDefault");
          }
          break;
        case "livemark":
          this._action = ACTION_ADD;
          this._itemType = LIVEMARK_CONTAINER;
          if ("feedURI" in dialogInfo)
            this._feedURI = dialogInfo.feedURI;
          if ("siteURI" in dialogInfo)
            this._siteURI = dialogInfo.siteURI;

          if (!this._itemTitle) {
            if (this._feedURI) {
              this._itemTitle =
                this._getURITitleFromHistory(this._feedURI);
              if (!this._itemTitle)
                this._itemTitle = this._feedURI.spec;
            }
            else
              this._itemTitle = this._strings.getString("newLivemarkDefault");
          }
      }

      if ("description" in dialogInfo)
        this._itemDescription = dialogInfo.description;
    }
    else { 
      const annos = PlacesUtils.annotations;
      const bookmarks = PlacesUtils.bookmarks;

      switch (dialogInfo.type) {
        case "bookmark":
          NS_ASSERT("bookmarkId" in dialogInfo);

          this._action = ACTION_EDIT;
          this._itemType = BOOKMARK_ITEM;
          this._bookmarkId = dialogInfo.bookmarkId;

          this._bookmarkURI = bookmarks.getBookmarkURI(this._bookmarkId);
          this._itemTitle = bookmarks.getItemTitle(this._bookmarkId);

          
          this._bookmarkKeyword =
            bookmarks.getKeywordForBookmark(this._bookmarkId);

          
          this._loadBookmarkInSidebar =
            annos.itemHasAnnotation(this._bookmarkId, LOAD_IN_SIDEBAR_ANNO);

          break;
        case "folder":
          NS_ASSERT("folderId" in dialogInfo);

          this._action = ACTION_EDIT;
          this._folderId = dialogInfo.folderId;

          const livemarks = PlacesUtils.livemarks;
          if (livemarks.isLivemark(this._folderId)) {
            this._itemType = LIVEMARK_CONTAINER;
            this._feedURI = livemarks.getFeedURI(this._folderId);
            this._siteURI = livemarks.getSiteURI(this._folderId);
          }
          else
            this._itemType = BOOKMARK_FOLDER;
          this._itemTitle = bookmarks.getItemTitle(this._folderId);
          break;
      }

      
      
      var itemId = dialogInfo.type == "bookmark" ? this._bookmarkId : this._folderId;
      if (annos.itemHasAnnotation(itemId, DESCRIPTION_ANNO)) {
        this._itemDescription = annos.getItemAnnotation(itemId,
                                                        DESCRIPTION_ANNO);
      }
    }
  },

  










  _getURITitleFromHistory: function BPP__getURITitleFromHistory(aURI) {
    NS_ASSERT(aURI instanceof Ci.nsIURI);

    
    return PlacesUtils.history.getPageTitle(aURI);
  },

  



  onDialogLoad: function BPP_onDialogLoad() {
    this._tm = window.opener.PlacesUtils.tm;

    this._determineItemInfo();
    this._populateProperties();
    this._forceHideRows();
    this.validateChanges();

    this._folderMenuList = this._element("folderMenuList");
    this._folderTree = this._element("folderTree");
    if (isElementVisible(this._folderMenuList))
      this._initFolderMenuList();

    window.sizeToContent();

    
    this._folderTreeHeight = parseInt(this._folderTree.getAttribute("height"));
  },

  







  _appendFolderItemToMenupopup:
  function BPP__appendFolderItemToMenupopup(aMenupopup, aFolderId) {
    try {
      var folderTitle = PlacesUtils.bookmarks.getItemTitle(aFolderId);
    }
    catch (ex) {
      NS_ASSERT(folderTitle, "no title found for folderId of " + aFolderId);
      return null;
    }

    
    this._element("foldersSeparator").hidden = false;

    var folderMenuItem = document.createElement("menuitem");
    folderMenuItem.folderId = aFolderId;
    folderMenuItem.setAttribute("label", folderTitle);
    folderMenuItem.className = "menuitem-iconic folder-icon";
    aMenupopup.appendChild(folderMenuItem);
    return folderMenuItem;
  },

  _initFolderMenuList: function BPP__initFolderMenuList() {
    
    var annos = PlacesUtils.annotations;
    var folderIds = annos.getItemsWithAnnotation(LAST_USED_ANNO, { });

    
    if (folderIds.length == 0) {
      this._element("foldersSeparator").hidden = true;
      return;
    }

    







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
    var menupopup = this._folderMenuList.menupopup;
    for (i=0; i < numberOfItems; i++) {
      this._appendFolderItemToMenupopup(menupopup, folders[i].folderId);
    }

    var defaultItem =
      this._getFolderMenuItem(this._defaultInsertionPoint.itemId, true);

    
    
    if (!defaultItem)
      defaultItem = this._element("bookmarksRootItem");

    this._folderMenuList.selectedItem = defaultItem;
  },

  QueryInterface: function BPP_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIMicrosummaryObserver) ||
        aIID.eqauls(Ci.nsISupports))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  _element: function BPP__element(aID) {
    return document.getElementById(aID);
  },

  



  _forceHideRows: function BPP__forceHideRows() {
    var hiddenRows = window.arguments[0].hiddenRows;
    if (!hiddenRows)
      return;

    if (hiddenRows.indexOf("title") != -1)
      this._element("namePicker").hidden = true;
    if (hiddenRows.indexOf("location") != -1)
      this._element("locationRow").hidden = true;
    if (hiddenRows.indexOf("keyword") != -1)
      this._element("keywordRow").hidden = true;
    if (hiddenRows.indexOf("description")!= -1)
      this._element("descriptionRow").hidden = true;
    if (hiddenRows.indexOf("folder picker") != -1)
      this._element("folderRow").hidden = true;
    if (hiddenRows.indexOf("feedURI") != -1)
      this._element("livemarkFeedLocationRow").hidden = true;
    if (hiddenRows.indexOf("siteURI") != -1)
      this._element("livemarkSiteLocationRow").hidden = true;
    if (hiddenRows.indexOf("load in sidebar") != -1)
      this._element("loadInSidebarCheckbox").hidden = true;
  },

  


  _populateProperties: function BPP__populateProperties() {
    document.title = this._getDialogTitle();
    document.documentElement.getButton("accept").label = this._getAcceptLabel();

    this._initNamePicker();
    this._element("descriptionTextfield").value = this._itemDescription;

    if (this._itemType == BOOKMARK_ITEM) {
      if (this._bookmarkURI)
        this._element("editURLBar").value = this._bookmarkURI.spec;

      if (typeof(this._bookmarkKeyword) == "string")
        this._element("keywordTextfield").value = this._bookmarkKeyword;

      if (this._loadBookmarkInSidebar)
        this._element("loadInSidebarCheckbox").checked = true;
    }
    else {
      this._element("locationRow").hidden = true;
      this._element("keywordRow").hidden = true;
      this._element("loadInSidebarCheckbox").hidden = true;
    }

    if (this._itemType == LIVEMARK_CONTAINER) {
      if (this._feedURI)
        this._element("feedLocationTextfield").value = this._feedURI.spec;
      if (this._siteURI)
        this._element("feedSiteLocationTextfield").value = this._siteURI.spec;
    }
    else {
      this._element("livemarkFeedLocationRow").hidden = true;
      this._element("livemarkSiteLocationRow").hidden = true;
    }

    if (this._action == ACTION_EDIT)
      this._element("folderRow").hidden = true;
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

  _initNamePicker: function BPP_initNamePicker() {
    var userEnteredNameField = this._element("userEnteredName");
    var namePicker = this._element("namePicker");
    userEnteredNameField.label = this._itemTitle;

    
    if (this._itemType != BOOKMARK_ITEM || !this._bookmarkURI) {
      namePicker.selectedItem = userEnteredNameField;
      return;
    }

    var itemToSelect = userEnteredNameField;
    try {
      this._microsummaries = this._mss.getMicrosummaries(this._bookmarkURI,
                                                         this._bookmarkId);
    }
    catch(ex) {
      
      
      
      
      
      
      this._microsummaries = null;
    }
    if (this._microsummaries) {
      var enumerator = this._microsummaries.Enumerate();

      if (enumerator.hasMoreElements()) {
        
        namePicker.setAttribute("droppable", "true");

        var menupopup = namePicker.menupopup;
        while (enumerator.hasMoreElements()) {
          var microsummary = enumerator.getNext()
                                       .QueryInterface(Ci.nsIMicrosummary);
          var menuItem = this._createMicrosummaryMenuItem(microsummary);

          if (this._action == ACTION_EDIT &&
              this._mss.isMicrosummary(this._bookmarkId, microsummary))
            itemToSelect = menuItem;

          menupopup.appendChild(menuItem);
        }
      }

      this._microsummaries.addObserver(this);
    }

    namePicker.selectedItem = itemToSelect;
  },

  
  onContentLoaded: function BPP_onContentLoaded(aMicrosummary) {
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

  onDialogUnload: function BPP_onDialogUnload() {
    if (this._microsummaries)
      this._microsummaries.removeObserver(this);

    
    if (!this._folderTree.collapsed) {
      this._folderTree.setAttribute("height",
                                    this._folderTree.boxObject.height);
    }
  },

  onDialogAccept: function BPP_onDialogAccept() {
    if (this._action == ACTION_ADD)
      this._createNewItem();
    else
      this._saveChanges();
  },

  





  validateChanges: function BPP_validateChanges() {
    document.documentElement.getButton("accept").disabled = !this._inputIsValid();
  },

  




  _inputIsValid: function BPP__inputIsValid() {
    if (this._itemType == BOOKMARK_ITEM && !this._containsValidURI("editURLBar"))
      return false;

    
    
    if (this._itemType == LIVEMARK_CONTAINER) {
      if (!this._containsValidURI("feedLocationTextfield"))
        return false;
      if (!this._containsValidURI("feedSiteLocationTextfield") &&
          (this._element("feedSiteLocationTextfield").value.length > 0))
        return false;
    }

    return true;
  },

  








  _containsValidURI: function BPP__containsValidURI(aTextboxID) {
    try {
      var value = this._element(aTextboxID).value;
      if (value) {
        var uri = PlacesUtils._uri(value);
        return true;
      }
    } catch (e) { }
    return false;
  },

  


  _getEditTitleTransaction:
  function BPP__getEditTitleTransaction(aItemId, aNewTitle) {
    return PlacesUtils.ptm.editItemTitle(aItemId, aNewTitle);
  },

  










  








  _getDescriptionAnnotation:
  function BPP__getDescriptionAnnotation(aDescription) {
    var anno = { name: DESCRIPTION_ANNO,
                 type: Ci.nsIAnnotationService.TYPE_STRING,
                 flags: 0,
                 value: aDescription,
                 expires: Ci.nsIAnnotationService.EXPIRE_NEVER };

    




    return anno;
  },

  









  _getLoadInSidebarAnnotation:
  function BPP__getLoadInSidebarAnnotation(aLoadInSidebar) {
    var anno = { name: LOAD_IN_SIDEBAR_ANNO,
                 type: Ci.nsIAnnotationService.TYPE_INT32,
                 flags: 0,
                 value: aLoadInSidebar,
                 expires: Ci.nsIAnnotationService.EXPIRE_NEVER };

    




    return anno;
  },

  





  _saveChanges: function BPP__saveChanges() {
    var itemId;
    if (this._itemType == BOOKMARK_ITEM)
      itemId = this._bookmarkId;
    else
      itemId = this._folderId;

    var transactions = [];

    
    var newTitle = this._element("userEnteredName").label;
    if (newTitle != this._itemTitle)
      transactions.push(this._getEditTitleTransaction(itemId, newTitle));

    
    var description = this._element("descriptionTextfield").value;
    if (description != this._itemDescription) {
      transactions.push(new PlacesEditItemDescriptionTransaction(
        itemId, description, this._itemType != BOOKMARK_ITEM));
    }

    if (this._itemType == BOOKMARK_ITEM) {
      
      var url = PlacesUtils._uri(this._element("editURLBar").value);
      if (!this._bookmarkURI.equals(url))
        transactions.push(PlacesUtils.ptm.editBookmarkURI(itemId, url));

      
      var newKeyword = this._element("keywordTextfield").value;
      if (newKeyword != this._bookmarkKeyword) {
        transactions.push(PlacesUtils.ptm.
                          editBookmarkKeyword(itemId, newKeyword));
      }

      
      var namePicker = this._element("namePicker");
      var newMicrosummary = namePicker.selectedItem.microsummary;

      
      
      
      
      
      if ((newMicrosummary == null && this._mss.hasMicrosummary(itemId)) ||
          (newMicrosummary != null &&
           !this._mss.isMicrosummary(itemId, newMicrosummary))) {
        transactions.push(
          PlacesUtils.ptm.editBookmarkMicrosummary(itemId, newMicrosummary));
      }

      
      var loadInSidebarChecked = this._element("loadInSidebarCheckbox").checked;
      if (loadInSidebarChecked != this._loadBookmarkInSidebar) {
        transactions.push(
          PlacesUtils.ptm.setLoadInSidebar(itemId, loadInSidebarChecked));
      }
    }
    else if (this._itemType == LIVEMARK_CONTAINER) {
      var feedURIString = this._element("feedLocationTextfield").value;
      var feedURI = PlacesUtils._uri(feedURIString);
      if (!this._feedURI.equals(feedURI)) {
        transactions.push(
          PlacesUtils.ptm.editLivemarkFeedURI(this._folderId, feedURI));
      }

      
      var newSiteURIString = this._element("feedSiteLocationTextfield").value;
      var newSiteURI = null;
      if (newSiteURIString)
        newSiteURI = PlacesUtils._uri(newSiteURIString);

      if ((!newSiteURI && this._siteURI)  ||
          (newSiteURI && (!this._siteURI || !this._siteURI.equals(newSiteURI)))) {
        transactions.push(
          PlacesUtils.ptm.editLivemarkSiteURI(this._folderId, newSiteURI));
      }
    }

    
    
    if (transactions.length > 0) {
      window.arguments[0].performed = true;
      var aggregate =
        PlacesUtils.ptm.aggregateTransactions(this._getDialogTitle(), transactions);
      this._tm.doTransaction(aggregate);
    }
  },

  






  _getInsertionPointDetails: function BPP__getInsertionPointDetails() {
    var containerId, indexInContainer = -1;
    if (isElementVisible(this._folderMenuList))
      containerId = this._getFolderIdFromMenuList();
    else {
      containerId = this._defaultInsertionPoint.itemId;
      indexInContainer = this._defaultInsertionPoint.index;
    }

    return [containerId, indexInContainer];
  },

  



  _getCreateNewBookmarkTransaction:
  function BPP__getCreateNewBookmarkTransaction(aContainer, aIndex) {
    var uri = PlacesUtils._uri(this._element("editURLBar").value);
    var title = this._element("userEnteredName").label;
    var keyword = this._element("keywordTextfield").value;
    var annotations = [];
    var description = this._element("descriptionTextfield").value;
    if (description)
      annotations.push(this._getDescriptionAnnotation(description));

    var loadInSidebar = this._element("loadInSidebarCheckbox").checked;
    if (loadInSidebar)
      annotations.push(this._getLoadInSidebarAnnotation(true));

    var childTransactions = [];
    var microsummary = this._element("namePicker").selectedItem.microsummary;
    if (microsummary) {
      childTransactions.push(
        PlacesUtils.ptm.editBookmarkMicrosummary(-1, microsummary));
    }

    var transactions = [PlacesUtils.ptm.createItem(uri, aContainer, aIndex,
                                                   title, keyword,
                                                   annotations,
                                                   childTransactions)];

    if (this._postData) {
      transactions.push(
        PlacesUtils.ptm.editURIPostData(uri, this._postData));
    }

    return PlacesUtils.ptm.aggregateTransactions(this._getDialogTitle(), transactions);
  },

  



  _getTransactionsForURIList: function BPP__getTransactionsForURIList() {
    var transactions = [];
    for (var i = 0; i < this._URIList.length; ++i) {
      var uri = this._URIList[i];
      var title = this._getURITitleFromHistory(uri);
      transactions.push(PlacesUtils.ptm.createItem(uri, -1, -1, title));
    }
    return transactions; 
  },

  



  _getCreateNewFolderTransaction:
  function BPP__getCreateNewFolderTransaction(aContainer, aIndex) {
    var folderName = this._element("namePicker").value;
    var annotations = [];
    var childItemsTransactions;
    if (this._URIList)
      childItemsTransactions = this._getTransactionsForURIList();
    var description = this._element("descriptionTextfield").value;
    if (description)
      annotations.push(this._getDescriptionAnnotation(description));

    return PlacesUtils.ptm.createFolder(folderName, aContainer, aIndex,
                                        annotations, childItemsTransactions);
  },

  



  _getCreateNewLivemarkTransaction:
  function BPP__getCreateNewLivemarkTransaction(aContainer, aIndex) {
    var feedURIString = this._element("feedLocationTextfield").value;
    var feedURI = PlacesUtils._uri(feedURIString);

    var siteURIString = this._element("feedSiteLocationTextfield").value;
    var siteURI = null;
    if (siteURIString)
      siteURI = PlacesUtils._uri(siteURIString);

    var name = this._element("namePicker").value;
    return PlacesUtils.ptm.createLivemark(feedURI, siteURI, name,
                                          aContainer, aIndex);
  },

  


  _createNewItem: function BPP__getCreateItemTransaction() {
    var [container, index] = this._getInsertionPointDetails();
    var createTxn;
    if (this._itemType == BOOKMARK_FOLDER)
      createTxn = this._getCreateNewFolderTransaction(container, index);
    else if (this._itemType == LIVEMARK_CONTAINER)
      createTxn = this._getCreateNewLivemarkTransaction(container, index);
    else 
      createTxn = this._getCreateNewBookmarkTransaction(container, index);

    
    this._markFolderAsRecentlyUsed(container);

    
    
    window.arguments[0].performed = true;
    this._tm.doTransaction(createTxn);
  },

  onNamePickerInput: function BPP_onNamePickerInput() {
    this._element("userEnteredName").label = this._element("namePicker").value;
  },

  toggleTreeVisibility: function BPP_toggleTreeVisibility() {
    var expander = this._element("expander");
    if (!this._folderTree.collapsed) { 
      expander.className = "down";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextdown"));
      document.documentElement.buttons = "accept,cancel";

      this._folderTreeHeight = this._folderTree.boxObject.height;
      this._folderTree.setAttribute("height", this._folderTreeHeight);
      this._folderTree.collapsed = true;
      resizeTo(window.outerWidth, window.outerHeight - this._folderTreeHeight);
    }
    else {
      expander.className = "up";
      expander.setAttribute("tooltiptext",
                            expander.getAttribute("tooltiptextup"));
      document.documentElement.buttons = "accept,cancel, extra2";

      if (!this._folderTree.treeBoxObject.view.isContainerOpen(0))
        this._folderTree.treeBoxObject.view.toggleOpenState(0);
      this._folderTree.selectFolders([this._getFolderIdFromMenuList()]);
      this._folderTree.focus();

      this._folderTree.collapsed = false;
      resizeTo(window.outerWidth, window.outerHeight + this._folderTreeHeight);
    }
  },

  _getFolderIdFromMenuList:
  function BPP__getFolderIdFromMenuList() {
    var selectedItem = this._folderMenuList.selectedItem
    switch (selectedItem.id) {
      case "bookmarksRootItem":
        return PlacesUtils.bookmarksRootId;
      case "toolbarFolderItem":
        return PlacesUtils.toolbarFolderId;
    }

    NS_ASSERT("folderId" in selectedItem,
              "Invalid menuitem in the folders-menulist");
    return selectedItem.folderId;
  },

  











  _getFolderMenuItem:
  function BPP__getFolderMenuItem(aFolderId, aCheckStaticFolderItems) {
    var menupopup = this._folderMenuList.menupopup;

    
    for (var i=3; i < menupopup.childNodes.length; i++) {
      if (menupopup.childNodes[i].folderId == aFolderId)
        return menupopup.childNodes[i];
    }

    if (aCheckStaticFolderItems) {
      if (aFolderId == PlacesUtils.bookmarksRootId)
        return this._element("bookmarksRootItem")
      if (aFolderId == PlacesUtils.toolbarFolderId)
        return this._element("toolbarFolderItem")
    }

    
    if (menupopup.childNodes.length == 3 + MAX_FOLDER_ITEM_IN_MENU_LIST)
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
    if ((oldSelectedItem.id == "toolbarFolderItem" &&
         folderId == PlacesUtils.bookmarks.toolbarFolder) ||
        (oldSelectedItem.id == "bookmarksRootItem" &&
         folderId == PlacesUtils.bookmarks.bookmarksRoot))
      return;

    var folderItem = this._getFolderMenuItem(folderId, false);
    this._folderMenuList.selectedItem = folderItem;
  },

  _markFolderAsRecentlyUsed:
  function BPP__markFolderAsRecentlyUsed(aFolderId) {
    
    
    PlacesUtils.annotations
               .setItemAnnotation(aFolderId, LAST_USED_ANNO,
                                  new Date().getTime(), 0,
                                  Ci.nsIAnnotationService.EXPIRE_NEVER);
  },

  newFolder: function BPP_newFolder() {
    
    this._folderTree.focus();
    goDoCommand("placesCmd_new:folder");
  }
};
