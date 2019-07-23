





























































































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

  _action: null,
  _itemType: null,
  _itemId: -1,
  _uri: null,
  _loadInSidebar: false,
  _title: "",
  _description: "",
  _URIs: [],
  _keyword: "",
  _postData: null,
  _charSet: "",
  _feedURI: null,
  _siteURI: null,

  _defaultInsertionPoint: null,
  _hiddenRows: [],
  _batching: false,

  



  _getAcceptLabel: function BPP__getAcceptLabel() {
    if (this._action == ACTION_ADD) {
      if (this._URIs.length)
        return this._strings.getString("dialogAcceptLabelAddMulti");

      if (this._itemType == LIVEMARK_CONTAINER)
        return this._strings.getString("dialogAcceptLabelAddLivemark");

      if (this._dummyItem || this._loadInSidebar)
        return this._strings.getString("dialogAcceptLabelAddItem");

      return this._strings.getString("dialogAcceptLabelSaveItem");
    }
    return this._strings.getString("dialogAcceptLabelEdit");
  },

  



  _getDialogTitle: function BPP__getDialogTitle() {
    if (this._action == ACTION_ADD) {
      if (this._itemType == BOOKMARK_ITEM)
        return this._strings.getString("dialogTitleAddBookmark");
      if (this._itemType == LIVEMARK_CONTAINER)
        return this._strings.getString("dialogTitleAddLivemark");

      
      NS_ASSERT(this._itemType == BOOKMARK_FOLDER, "Unknown item type");
      if (this._URIs.length)
        return this._strings.getString("dialogTitleAddMulti");

      return this._strings.getString("dialogTitleAddFolder");
    }
    if (this._action == ACTION_EDIT) {
      return this._strings.getFormattedString("dialogTitleEdit", [this._title]);
    }
    return "";
  },

  


  _determineItemInfo: function BPP__determineItemInfo() {
    var dialogInfo = window.arguments[0];
    this._action = dialogInfo.action == "add" ? ACTION_ADD : ACTION_EDIT;
    this._hiddenRows = dialogInfo.hiddenRows ? dialogInfo.hiddenRows : [];
    if (this._action == ACTION_ADD) {
      NS_ASSERT("type" in dialogInfo, "missing type property for add action");

      if ("title" in dialogInfo)
        this._title = dialogInfo.title;

      if ("defaultInsertionPoint" in dialogInfo) {
        this._defaultInsertionPoint = dialogInfo.defaultInsertionPoint;
      }
      else
        this._defaultInsertionPoint =
          new InsertionPoint(PlacesUtils.bookmarksMenuFolderId,
                             PlacesUtils.bookmarks.DEFAULT_INDEX,
                             Ci.nsITreeView.DROP_ON);

      switch(dialogInfo.type) {
        case "bookmark":
          this._itemType = BOOKMARK_ITEM;
          if ("uri" in dialogInfo) {
            NS_ASSERT(dialogInfo.uri instanceof Ci.nsIURI,
                      "uri property should be a uri object");
            this._uri = dialogInfo.uri;
            if (typeof(this._title) != "string") {
              this._title = this._getURITitleFromHistory(this._uri) ||
                            this._uri.spec;
            }
          }
          else {
            this._uri = PlacesUtils._uri("about:blank");
            this._title = this._strings.getString("newBookmarkDefault");
            this._dummyItem = true;
          }

          if ("loadBookmarkInSidebar" in dialogInfo)
            this._loadInSidebar = dialogInfo.loadBookmarkInSidebar;

          if ("keyword" in dialogInfo) {
            this._keyword = dialogInfo.keyword;
            if ("postData" in dialogInfo)
              this._postData = dialogInfo.postData;
            if ("charSet" in dialogInfo)
              this._charSet = dialogInfo.charSet;
          }
          break;

        case "folder":
          this._itemType = BOOKMARK_FOLDER;
          if (!this._title) {
            if ("URIList" in dialogInfo) {
              this._title = this._strings.getString("bookmarkAllTabsDefault");
              this._URIs = dialogInfo.URIList;
            }
            else
              this._title = this._strings.getString("newFolderDefault");
              this._dummyItem = true;
          }
          break;

        case "livemark":
          this._itemType = LIVEMARK_CONTAINER;
          if ("feedURI" in dialogInfo)
            this._feedURI = dialogInfo.feedURI;
          if ("siteURI" in dialogInfo)
            this._siteURI = dialogInfo.siteURI;

          if (!this._title) {
            if (this._feedURI) {
              this._title = this._getURITitleFromHistory(this._feedURI) ||
                            this._feedURI.spec;
            }
            else
              this._title = this._strings.getString("newLivemarkDefault");
          }
      }

      if ("description" in dialogInfo)
        this._description = dialogInfo.description;
    }
    else { 
      NS_ASSERT("itemId" in dialogInfo);
      this._itemId = dialogInfo.itemId;
      this._title = PlacesUtils.bookmarks.getItemTitle(this._itemId);
      
      this._hiddenRows.push("folderPicker");

      switch (dialogInfo.type) {
        case "bookmark":
          this._itemType = BOOKMARK_ITEM;

          this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);
          
          this._keyword = PlacesUtils.bookmarks
                                     .getKeywordForBookmark(this._itemId);
          
          this._loadInSidebar = PlacesUtils.annotations
                                           .itemHasAnnotation(this._itemId,
                                                              LOAD_IN_SIDEBAR_ANNO);
          break;

        case "folder":
          if (PlacesUtils.livemarks.isLivemark(this._itemId)) {
            this._itemType = LIVEMARK_CONTAINER;
            this._feedURI = PlacesUtils.livemarks.getFeedURI(this._itemId);
            this._siteURI = PlacesUtils.livemarks.getSiteURI(this._itemId);
          }
          else
            this._itemType = BOOKMARK_FOLDER;
          break;
      }

      
      if (PlacesUtils.annotations
                     .itemHasAnnotation(this._itemId, DESCRIPTION_ANNO)) {
        this._description = PlacesUtils.annotations
                                       .getItemAnnotation(this._itemId,
                                                          DESCRIPTION_ANNO);
      }
    }
  },

  










  _getURITitleFromHistory: function BPP__getURITitleFromHistory(aURI) {
    NS_ASSERT(aURI instanceof Ci.nsIURI);

    
    return PlacesUtils.history.getPageTitle(aURI);
  },

  



  onDialogLoad: function BPP_onDialogLoad() {
    this._determineItemInfo();

    document.title = this._getDialogTitle();
    var acceptButton = document.documentElement.getButton("accept");
    acceptButton.label = this._getAcceptLabel();

    this._beginBatch();

    switch (this._action) {
      case ACTION_EDIT:
        this._fillEditProperties();
        break;
      case ACTION_ADD:
        this._fillAddProperties();
        
        
        if (this._itemType == BOOKMARK_ITEM ||
            this._itemType == LIVEMARK_CONTAINER)
          acceptButton.disabled = !this._inputIsValid();
        break;
    }

    
    
    
    
    
    if (!this._element("tagsRow").collapsed) {
      this._element("tagsSelectorRow")
          .addEventListener("DOMAttrModified", this, false);
      
      
      document.addEventListener("keypress", this, true);
    }
    if (!this._element("folderRow").collapsed) {
      this._element("folderTreeRow")
          .addEventListener("DOMAttrModified", this, false);
    }

    
    if (this._itemType == BOOKMARK_ITEM) {
      this._element("locationField")
          .addEventListener("input", this, false);
    }
    else if (this._itemType == LIVEMARK_CONTAINER) {
      this._element("feedLocationField")
          .addEventListener("input", this, false);
      this._element("siteLocationField")
          .addEventListener("input", this, false);
    }

    window.sizeToContent();
  },

  
  _elementsHeight: [],
  handleEvent: function BPP_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "keypress":
        if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN &&
            aEvent.target.localName != "tree" &&
            aEvent.target.className != "expander-up" &&
            aEvent.target.className != "expander-down" &&
            !aEvent.target.popupOpen) {
          
          
          document.documentElement.acceptDialog();
        }
        break;

      case "input":
        if (aEvent.target.id == "editBMPanel_locationField" ||
            aEvent.target.id == "editBMPanel_feedLocationField" ||
            aEvent.target.id == "editBMPanel_siteLocationField") {
          
          document.documentElement
                  .getButton("accept").disabled = !this._inputIsValid();
        }
        break;

      case "DOMAttrModified":
        
        
        if ((aEvent.target.id == "editBMPanel_tagsSelectorRow" ||
             aEvent.target.id == "editBMPanel_folderTreeRow") &&
            aEvent.attrName == "collapsed" &&
            aEvent.target == aEvent.originalTarget) {
          var element = aEvent.target;
          var id = element.id;
          var newHeight = window.outerHeight;
          if (aEvent.newValue) 
            newHeight -= this._elementsHeight[id];
          else {
            this._elementsHeight[id] = element.boxObject.height;
            newHeight += this._elementsHeight[id];
          }

          window.resizeTo(window.outerWidth, newHeight);
        }
        break;
    }
  },

  _beginBatch: function BPP__beginBatch() {
    if (this._batching)
      return;

    PlacesUIUtils.ptm.beginBatch();
    this._batching = true;

    
    
    
    
    
    PlacesUIUtils.ptm.doTransaction({ doTransaction: function() { },
                                      undoTransaction: function() { },
                                      redoTransaction: function() { },
                                      isTransient: false,
                                      merge: function() { return false; } });
  },

  _endBatch: function BPP__endBatch() {
    if (!this._batching)
      return;

    PlacesUIUtils.ptm.endBatch();
    this._batching = false;
  },

  _fillEditProperties: function BPP__fillEditProperties() {
    gEditItemOverlay.initPanel(this._itemId,
                               { hiddenRows: this._hiddenRows });
  },

  _fillAddProperties: function BPP__fillAddProperties() {
    this._createNewItem();
    
    gEditItemOverlay.initPanel(this._itemId,
                               { hiddenRows: this._hiddenRows });
    
    
    
    var locationField = this._element("locationField");
    if (locationField.value == "about:blank")
      locationField.value = "";
  },

  
  QueryInterface: function BPP_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_NOINTERFACE;
  },

  _element: function BPP__element(aID) {
    return document.getElementById("editBMPanel_" + aID);
  },

  onDialogUnload: function BPP_onDialogUnload() {
    
    
    
    this._element("tagsSelectorRow")
        .removeEventListener("DOMAttrModified", this, false);
    document.removeEventListener("keypress", this, true);
    this._element("folderTreeRow")
        .removeEventListener("DOMAttrModified", this, false);
    this._element("locationField")
        .removeEventListener("input", this, false);
    this._element("feedLocationField")
        .removeEventListener("input", this, false);
    this._element("siteLocationField")
        .removeEventListener("input", this, false);
  },

  onDialogAccept: function BPP_onDialogAccept() {
    
    document.commandDispatcher.focusedElement.blur();
    
    
    gEditItemOverlay.uninitPanel(true);
    gEditItemOverlay = null;
    this._endBatch();
    window.arguments[0].performed = true;
  },

  onDialogCancel: function BPP_onDialogCancel() {
    
    
    
    gEditItemOverlay.uninitPanel(true);
    gEditItemOverlay = null;
    this._endBatch();
    PlacesUIUtils.ptm.undoTransaction();
    window.arguments[0].performed = false;
  },

  




  _inputIsValid: function BPP__inputIsValid() {
    if (this._itemType == BOOKMARK_ITEM &&
        !this._containsValidURI("locationField"))
      return false;

    
    
    if (this._itemType == LIVEMARK_CONTAINER) {
      if (!this._containsValidURI("feedLocationField"))
        return false;
      if (!this._containsValidURI("siteLocationField") &&
          (this._element("siteLocationField").value.length > 0))
        return false;
    }

    return true;
  },

  








  _containsValidURI: function BPP__containsValidURI(aTextboxID) {
    try {
      var value = this._element(aTextboxID).value;
      if (value) {
        var uri = PlacesUIUtils.createFixedURI(value);
        return true;
      }
    } catch (e) { }
    return false;
  },

  






  _getInsertionPointDetails: function BPP__getInsertionPointDetails() {
    var containerId = this._defaultInsertionPoint.itemId;
    var indexInContainer = this._defaultInsertionPoint.index;

    return [containerId, indexInContainer];
  },

  



  _getCreateNewBookmarkTransaction:
  function BPP__getCreateNewBookmarkTransaction(aContainer, aIndex) {
    var annotations = [];
    var childTransactions = [];

    if (this._description) {
      childTransactions.push(
        PlacesUIUtils.ptm.editItemDescription(-1, this._description));
    }

    if (this._loadInSidebar) {
      childTransactions.push(
        PlacesUIUtils.ptm.setLoadInSidebar(-1, this._loadInSidebar));
    }

    if (this._postData) {
      childTransactions.push(
        PlacesUIUtils.ptm.editBookmarkPostData(-1, this._postData));
    }

    
    if (this._charSet)
      PlacesUtils.history.setCharsetForURI(this._uri, this._charSet);

    var transactions = [PlacesUIUtils.ptm.createItem(this._uri,
                                                     aContainer, aIndex,
                                                     this._title, this._keyword,
                                                     annotations,
                                                     childTransactions)];

    return PlacesUIUtils.ptm.aggregateTransactions(this._getDialogTitle(),
                                                   transactions);
  },

  



  _getTransactionsForURIList: function BPP__getTransactionsForURIList() {
    var transactions = [];
    for (var i = 0; i < this._URIs.length; ++i) {
      var uri = this._URIs[i];
      var title = this._getURITitleFromHistory(uri);
      transactions.push(PlacesUIUtils.ptm.createItem(uri, -1, -1, title));
    }
    return transactions; 
  },

  



  _getCreateNewFolderTransaction:
  function BPP__getCreateNewFolderTransaction(aContainer, aIndex) {
    var annotations = [];
    var childItemsTransactions;
    if (this._URIs.length)
      childItemsTransactions = this._getTransactionsForURIList();

    if (this._description)
      annotations.push(this._getDescriptionAnnotation(this._description));

    return PlacesUIUtils.ptm.createFolder(this._title, aContainer, aIndex,
                                          annotations, childItemsTransactions);
  },

  



  _getCreateNewLivemarkTransaction:
  function BPP__getCreateNewLivemarkTransaction(aContainer, aIndex) {
    return PlacesUIUtils.ptm.createLivemark(this._feedURI, this._siteURI,
                                            this._title,
                                            aContainer, aIndex);
  },

  


  _createNewItem: function BPP__getCreateItemTransaction() {
    var [container, index] = this._getInsertionPointDetails();
    var txn;

    switch (this._itemType) {
      case BOOKMARK_FOLDER:
        txn = this._getCreateNewFolderTransaction(container, index);
        break;
      case LIVEMARK_CONTAINER:
        txn = this._getCreateNewLivemarkTransaction(container, index);
        break;      
      default: 
        txn = this._getCreateNewBookmarkTransaction(container, index);
    }

    PlacesUIUtils.ptm.doTransaction(txn);
    this._itemId = PlacesUtils.bookmarks.getIdForItemAt(container, index);
  },

  _getFolderIdFromMenuList:
  function BPP__getFolderIdFromMenuList() {
    var selectedItem = this._element("folderPicker").selectedItem;
    NS_ASSERT("folderId" in selectedItem,
              "Invalid menuitem in the folders-menulist");
    return selectedItem.folderId;
  },

  







  _getFolderMenuItem:
  function BPP__getFolderMenuItem(aFolderId) {
    var menupopup = this._folderMenuList.menupopup;

    for (var i = 0; i < menupopup.childNodes.length; i++) {
      if (menupopup.childNodes[i].folderId == aFolderId)
        return menupopup.childNodes[i];
    }

    
    if (menupopup.childNodes.length == 3 + MAX_FOLDER_ITEM_IN_MENU_LIST)
      menupopup.removeChild(menupopup.lastChild);

    return this._appendFolderItemToMenupopup(menupopup, aFolderId);
  }
};
