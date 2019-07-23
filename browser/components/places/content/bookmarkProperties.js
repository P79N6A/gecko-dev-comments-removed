
























































































const BOOKMARK_ITEM = 0;
const BOOKMARK_FOLDER = 1;
const LIVEMARK_CONTAINER = 2;

const ACTION_EDIT = 0;
const ACTION_ADD = 1;
const ACTION_ADD_WITH_ITEMS = 2;








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
  _bookmarkId: null,
  _bookmarkURI: null,
  _loadBookmarkInSidebar: false,
  _itemTitle: "",
  _itemDescription: "",
  _microsummaries: null,

  



  _getAcceptLabel: function BPP__getAcceptLabel() {
    if (this._action == ACTION_ADD)
      return this._strings.getString("dialogAcceptLabelAdd");
    if (this._action == ACTION_ADD_WITH_ITEMS)
      return this._strings.getString("dialogAcceptLabelAddMulti");

    return this._strings.getString("dialogAcceptLabelEdit");
  },

  



  _getDialogTitle: function BPP__getDialogTitle() {
    if (this._action == ACTION_ADD) {
      if (this._itemType == BOOKMARK_ITEM)
        return this._strings.getString("dialogTitleAddBookmark");
      if (this._itemType == LIVEMARK_CONTAINER)
        return this._strings.getString("dialogTitleAddLivemark");

      
      NS_ASSERT(this._itemType == BOOKMARK_FOLDER, "bogus item type");
      return this._strings.getString("dialogTitleAddFolder");
    }
    if (this._action == ACTION_ADD_WITH_ITEMS)
      return this._strings.getString("dialogTitleAddMulti");
    if (this._action == ACTION_EDIT) {
      return this._strings
                 .getFormattedString("dialogTitleEdit", [this._itemTitle]);
    }
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

      switch(dialogInfo.type) {
        case "bookmark":
          this._action = ACTION_ADD;
          this._itemType = BOOKMARK_ITEM;
          if ("uri" in dialogInfo) {
            NS_ASSERT(dialogInfo.uri instanceof Ci.nsIURI,
                      "uri property should be a uri object");
            this._bookmarkURI = dialogInfo.uri;
          }
          if (!this._itemTitle) {
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

          break;
        case "folder":
          this._action = ACTION_ADD;
          this._itemType = BOOKMARK_FOLDER;
          if (!this._itemTitle)
            this._itemTitle = this._strings.getString("newFolderDefault");
          break;
        case "folder with items":
          NS_ASSERT("URIList" in dialogInfo,
                    "missing URLList property for 'folder with items' action");
          this._action = ACTION_ADD_WITH_ITEMS
          this._itemType = BOOKMARK_FOLDER;
          this._URIList = dialogInfo.URIList;
          if (!this._itemTitle) {
            this._itemTitle =
              this._strings.getString("bookmarkAllTabsDefault");
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

      var placeURI;
      switch (dialogInfo.type) {
        case "bookmark":
          NS_ASSERT("bookmarkId" in dialogInfo);

          this._action = ACTION_EDIT;
          this._itemType = BOOKMARK_ITEM;
          this._bookmarkId = dialogInfo.bookmarkId;
          placeURI = bookmarks.getItemURI(this._bookmarkId);

          this._bookmarkURI = bookmarks.getBookmarkURI(this._bookmarkId);
          this._itemTitle = bookmarks.getItemTitle(this._bookmarkId);

          
          this._bookmarkKeyword =
            bookmarks.getKeywordForBookmark(this._bookmarkId);

          
          this._loadBookmarkInSidebar =
            annos.hasAnnotation(placeURI, LOAD_IN_SIDEBAR_ANNO);

          break;
        case "folder":
          NS_ASSERT("folderId" in dialogInfo);

          this._action = ACTION_EDIT;
          this._folderId = dialogInfo.folderId;
          placeURI = bookmarks.getFolderURI(this._folderId);

          const livemarks = PlacesUtils.livemarks;
          if (livemarks.isLivemark(this._folderId)) {
            this._itemType = LIVEMARK_CONTAINER;
            this._feedURI = livemarks.getFeedURI(this._folderId);
            this._siteURI = livemarks.getSiteURI(this._folderId);
          }
          else
            this._itemType = BOOKMARK_FOLDER;
          this._itemTitle = bookmarks.getFolderTitle(this._folderId);
          break;
      }

      
      if (annos.hasAnnotation(placeURI, DESCRIPTION_ANNO)) {
        this._itemDescription =
          annos.getAnnotationString(placeURI, DESCRIPTION_ANNO);
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
    
    
    
    var self = this;
    setTimeout(function() { self._initFolderTree(); }, 0);
    
    this._populateProperties();
    this._forceHideRows();
    this.validateChanges();
    this._updateSize();
  },
  
  


  _initFolderTree: function BPP__initFolderTree() {
    this._folderTree = this._element("folderTree");
    this._folderTree.peerDropTypes = [];
    this._folderTree.childDropTypes = [];
    if (isElementVisible(this._folderTree)) {
      if (this._defaultInsertionPoint)
        this._folderTree.selectFolders([this._defaultInsertionPoint.folderId]);
      else
        this._folderTree.selectFolders([PlacesUtils.bookmarks.bookmarksRoot]);
    }
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
      this._element("shortcutRow").hidden = true;
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

      if (this._bookmarkKeyword)
        this._element("keywordTextfield").value = this._bookmarkKeyword;

      if (this._loadBookmarkInSidebar)
        this._element("loadInSidebarCheckbox").checked = true;
    }
    else {
      this._element("locationRow").hidden = true;
      this._element("shortcutRow").hidden = true;
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
    var placeURI = null;
    if (this._action == ACTION_EDIT) {
      NS_ASSERT(this._bookmarkId, "No bookmark identifier");
      placeURI = PlacesUtils.bookmarks.getItemURI(this._bookmarkId);
    }
    try {
      this._microsummaries = this._mss.getMicrosummaries(this._bookmarkURI,
                                                         placeURI);
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
              this._mss.isMicrosummary(placeURI, microsummary))
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

  


  _updateSize: function BPP__updateSize() {
    var width = window.outerWidth;
    window.sizeToContent();
    window.resizeTo(width, window.outerHeight);
  },

  onDialogUnload: function BPP_onDialogUnload() {
    if (this._microsummaries)
      this._microsummaries.removeObserver(this);
  },

  onDialogAccept: function BPP_onDialogAccept() {
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
    
    if (this._itemType == BOOKMARK_ITEM)
      return new PlacesEditItemTitleTransaction(aItemId, aNewTitle);

    
    return new PlacesEditFolderTitleTransaction(aItemId, aNewTitle);
  },

  


  _getCreateItemTransaction: function() {
    NS_ASSERT(this._action != ACTION_EDIT,
              "_getCreateItemTransaction called when editing an item");

    var containerId, indexInContainer = -1;
    if (isElementVisible(this._folderTree))
      containerId =  asFolder(this._folderTree.selectedNode).folderId;
    else if (this._defaultInsertionPoint) {
      containerId = this._defaultInsertionPoint.folderId;
      indexInContainer = this._defaultInsertionPoint.index;
    }
    else
      containerId = PlacesUtils.bookmarks.bookmarksRoot;

    if (this._itemType == BOOKMARK_ITEM) {
      var uri = PlacesUtils._uri(this._element("editURLBar").value);
      NS_ASSERT(uri, "cannot create an item without a uri");
      return new
        PlacesCreateItemTransaction(uri, containerId, indexInContainer);
    }
    else if (this._itemType == LIVEMARK_CONTAINER) {
      var feedURIString = this._element("feedLocationTextfield").value;
      var feedURI = PlacesUtils._uri(feedURIString);

      var siteURIString = this._element("feedSiteLocationTextfield").value;
      var siteURI = null;
      if (siteURIString)
        siteURI = PlacesUtils._uri(siteURIString);

      var name = this._element("namePicker").value;
      return new PlacesCreateLivemarkTransaction(feedURI, siteURI,
                                                 name, containerId,
                                                 indexInContainer);
    }
    else if (this._itemType == BOOKMARK_FOLDER) { 
      var name = this._element("namePicker").value;
      return new PlacesCreateFolderTransaction(name, containerId,
                                               indexInContainer);
    }
  },

  



  _saveChanges: function BPP__saveChanges() {
    var transactions = [];
    var childItemsTransactions = [];

    
    var itemId = -1;
    if (this._action == ACTION_EDIT) {
      if (this._itemType == BOOKMARK_ITEM)
        itemId = this._bookmarkId;
      else
        itemId = this._folderId;
    }

    
    if (this._action == ACTION_EDIT && this._itemType == BOOKMARK_ITEM) {
      var url = PlacesUtils._uri(this._element("editURLBar").value);
      if (!this._bookmarkURI.equals(url))
        transactions.push(new PlacesEditBookmarkURITransaction(itemId, url));
    }

    
    
    
    var newTitle = this._element("userEnteredName").label;
    if (this._action != ACTION_EDIT || newTitle != this._itemTitle)
      transactions.push(this._getEditTitleTransaction(itemId, newTitle));

    
    if (this._itemType == BOOKMARK_ITEM) {
      var newKeyword = this._element("keywordTextfield").value;
      if (this._action != ACTION_EDIT || newKeyword != this._bookmarkKeyword) {
        transactions.push(
          new PlacesEditBookmarkKeywordTransaction(itemId, newKeyword));
      }
    }

    
    if (this._action == ACTION_ADD_WITH_ITEMS) {
      for (var i = 0; i < this._URIList.length; ++i) {
        var uri = this._URIList[i];
        var title = this._getURITitleFromHistory(uri);
        var txn = new PlacesCreateItemTransaction(uri, -1, -1);
        txn.childTransactions.push(
          new PlacesEditItemTitleTransaction(-1, title));
        childItemsTransactions.push(txn);
      }
    }

    
    if (this._action == ACTION_EDIT && this._itemType == LIVEMARK_CONTAINER) {
      var feedURIString = this._element("feedLocationTextfield").value;
      var feedURI = PlacesUtils._uri(feedURIString);
      if (!this._feedURI.equals(feedURI)) {
        transactions.push(
          new PlacesEditLivemarkFeedURITransaction(this._folderId, feedURI));
      }

      
      var siteURIString = this._element("feedSiteLocationTextfield").value;
      var siteURI = null;
      if (siteURIString)
        siteURI = PlacesUtils._uri(siteURIString);

      if ((!siteURI && this._siteURI)  ||
          (siteURI && !this._siteURI.equals(siteURI))) {
        transactions.push(
          new PlacesEditLivemarkSiteURITransaction(this._folderId, siteURI));
      }
    }

    
    if (this._itemType == BOOKMARK_ITEM) {
      var namePicker = this._element("namePicker");

      
      
      
      if (namePicker.selectedIndex == -1)
        namePicker.selectedIndex = 0;

      
      
      var newMicrosummary = namePicker.selectedItem.microsummary;

      if (this._action == ACTION_ADD && newMicrosummary) {
        transactions.push(
          new PlacesEditBookmarkMicrosummaryTransaction(itemId,
                                                        newMicrosummary));
      }
      else if (this._action == ACTION_EDIT) {
        NS_ASSERT(itemId != -1, "should have had a real bookmark id");

        
        
        
        
        
        var placeURI = PlacesUtils.bookmarks.getItemURI(itemId);
        if ((newMicrosummary == null && this._mss.hasMicrosummary(placeURI)) ||
            (newMicrosummary != null &&
             !this._mss.isMicrosummary(placeURI, newMicrosummary))) {
          transactions.push(
            new PlacesEditBookmarkMicrosummaryTransaction(itemId,
                                                          newMicrosummary));
        }
      }
    }

    
    if (this._itemType == BOOKMARK_ITEM) {
      var checked = this._element("loadInSidebarCheckbox").checked;
      if (this._action == ACTION_ADD ||
          checked != this._loadBookmarkInSidebar) {
        transactions.push(
          new PlacesSetLoadInSidebarTransaction(itemId, checked));
      }
    }

    
    var description = this._element("descriptionTextfield").value;
    if ((this._action != ACTION_EDIT && description) ||
        (description != this._itemDescription)) {
      var isFolder = this._itemType != BOOKMARK_ITEM;
      transactions.push(new PlacesEditItemDescriptionTransaction(
        itemId, description, this._itemType != BOOKMARK_ITEM));
    }

    
    
    if (transactions.length > 0) {
      window.arguments[0].performed = true;

      if (this._action != ACTION_EDIT) {
        var createTxn = this._getCreateItemTransaction();
        NS_ASSERT(createTxn, "failed to get a create-item transaction");

        
        createTxn.childTransactions =
          createTxn.childTransactions.concat(transactions);

        if (this._action == ACTION_ADD_WITH_ITEMS) {
          
          createTxn.childItemsTransactions =
            createTxn.childItemsTransactions.concat(childItemsTransactions);
        }
        this._tm.doTransaction(createTxn);
      }
      else {
        
        var aggregate =
          new PlacesAggregateTransaction(this._getDialogTitle(), transactions);
        this._tm.doTransaction(aggregate);
      }
    }
  },

  onNamePickerInput: function BPP_onNamePickerInput() {
    this._element("userEnteredName").label = this._element("namePicker").value;
  }
};
