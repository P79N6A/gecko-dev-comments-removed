# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:




var StarUI = {
  _itemId: -1,
  uri: null,
  _batching: false,

  _element: function(aID) {
    return document.getElementById(aID);
  },

  
  get panel() {
    delete this.panel;
    var element = this._element("editBookmarkPanel");
    
    
    element.hidden = false;
    element.addEventListener("popuphidden", this, false);
    element.addEventListener("keypress", this, false);
    return this.panel = element;
  },

  
  get _blockedCommands() {
    delete this._blockedCommands;
    return this._blockedCommands =
      ["cmd_close", "cmd_closeWindow"].map(function (id) this._element(id), this);
  },

  _blockCommands: function SU__blockCommands() {
    this._blockedCommands.forEach(function (elt) {
      
      if (elt.hasAttribute("wasDisabled"))
        return;
      if (elt.getAttribute("disabled") == "true") {
        elt.setAttribute("wasDisabled", "true");
      } else {
        elt.setAttribute("wasDisabled", "false");
        elt.setAttribute("disabled", "true");
      }
    });
  },

  _restoreCommandsState: function SU__restoreCommandsState() {
    this._blockedCommands.forEach(function (elt) {
      if (elt.getAttribute("wasDisabled") != "true")
        elt.removeAttribute("disabled");
      elt.removeAttribute("wasDisabled");
    });
  },

  
  handleEvent: function SU_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "popuphidden":
        if (aEvent.originalTarget == this.panel) {
          if (!this._element("editBookmarkPanelContent").hidden)
            this.quitEditMode();

          this._restoreCommandsState();
          this._itemId = -1;
          if (this._batching) {
            PlacesUtils.transactionManager.endBatch(false);
            this._batching = false;
          }

          switch (this._actionOnHide) {
            case "cancel": {
              PlacesUtils.transactionManager.undoTransaction();
              break;
            }
            case "remove": {
              
              
              PlacesUtils.transactionManager.beginBatch(null);
              let itemIds = PlacesUtils.getBookmarksForURI(this._uriForRemoval);
              for (let i = 0; i < itemIds.length; i++) {
                let txn = new PlacesRemoveItemTransaction(itemIds[i]);
                PlacesUtils.transactionManager.doTransaction(txn);
              }
              PlacesUtils.transactionManager.endBatch(false);
              break;
            }
          }
          this._actionOnHide = "";
        }
        break;
      case "keypress":
        if (aEvent.defaultPrevented) {
          
          break;
        }
        switch (aEvent.keyCode) {
          case KeyEvent.DOM_VK_ESCAPE:
            if (!this._element("editBookmarkPanelContent").hidden)
              this.cancelButtonOnCommand();
            break;
          case KeyEvent.DOM_VK_RETURN:
            if (aEvent.target.classList.contains("expander-up") ||
                aEvent.target.classList.contains("expander-down") ||
                aEvent.target.id == "editBMPanel_newFolderButton")  {
              
              
              break;
            }
            this.panel.hidePopup();
            break;
        }
        break;
    }
  },

  _overlayLoaded: false,
  _overlayLoading: false,
  showEditBookmarkPopup:
  function SU_showEditBookmarkPopup(aItemId, aAnchorElement, aPosition) {
    
    
    if (this._overlayLoading)
      return;

    if (this._overlayLoaded) {
      this._doShowEditBookmarkPanel(aItemId, aAnchorElement, aPosition);
      return;
    }

    this._overlayLoading = true;
    document.loadOverlay(
      "chrome://browser/content/places/editBookmarkOverlay.xul",
      (function (aSubject, aTopic, aData) {
        
        
        let header = this._element("editBookmarkPanelHeader");
        let rows = this._element("editBookmarkPanelGrid").lastChild;
        rows.insertBefore(header, rows.firstChild);
        header.hidden = false;

        this._overlayLoading = false;
        this._overlayLoaded = true;
        this._doShowEditBookmarkPanel(aItemId, aAnchorElement, aPosition);
      }).bind(this)
    );
  },

  _doShowEditBookmarkPanel:
  function SU__doShowEditBookmarkPanel(aItemId, aAnchorElement, aPosition) {
    if (this.panel.state != "closed")
      return;

    this._blockCommands(); 

    
    
    
    
    this._element("editBookmarkPanelTitle").value =
      this._batching ?
        gNavigatorBundle.getString("editBookmarkPanel.pageBookmarkedTitle") :
        gNavigatorBundle.getString("editBookmarkPanel.editBookmarkTitle");

    
    this._element("editBookmarkPanelDescription").textContent = "";
    this._element("editBookmarkPanelBottomButtons").hidden = false;
    this._element("editBookmarkPanelContent").hidden = false;

    
    
    this._element("editBookmarkPanelRemoveButton").hidden = this._batching;

    
    
    var bookmarks = PlacesUtils.getBookmarksForURI(gBrowser.currentURI);
    var forms = gNavigatorBundle.getString("editBookmark.removeBookmarks.label");
    var label = PluralForm.get(bookmarks.length, forms).replace("#1", bookmarks.length);
    this._element("editBookmarkPanelRemoveButton").label = label;

    
    this._element("editBookmarkPanelStarIcon").removeAttribute("unstarred");

    this._itemId = aItemId !== undefined ? aItemId : this._itemId;
    this.beginBatch();

    this.panel.openPopup(aAnchorElement, aPosition);

    gEditItemOverlay.initPanel(this._itemId,
                               { hiddenRows: ["description", "location",
                                              "loadInSidebar", "keyword"] });
  },

  panelShown:
  function SU_panelShown(aEvent) {
    if (aEvent.target == this.panel) {
      if (!this._element("editBookmarkPanelContent").hidden) {
        let fieldToFocus = "editBMPanel_" +
          gPrefService.getCharPref("browser.bookmarks.editDialog.firstEditField");
        var elt = this._element(fieldToFocus);
        elt.focus();
        elt.select();
      }
      else {
        
        
        this.panel.focus();
      }
    }
  },

  quitEditMode: function SU_quitEditMode() {
    this._element("editBookmarkPanelContent").hidden = true;
    this._element("editBookmarkPanelBottomButtons").hidden = true;
    gEditItemOverlay.uninitPanel(true);
  },

  cancelButtonOnCommand: function SU_cancelButtonOnCommand() {
    this._actionOnHide = "cancel";
    this.panel.hidePopup();
  },

  removeBookmarkButtonCommand: function SU_removeBookmarkButtonCommand() {
    this._uriForRemoval = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);
    this._actionOnHide = "remove";
    this.panel.hidePopup();
  },

  beginBatch: function SU_beginBatch() {
    if (!this._batching) {
      PlacesUtils.transactionManager.beginBatch(null);
      this._batching = true;
    }
  }
}




var PlacesCommandHook = {
  









  
  bookmarkPage: function PCH_bookmarkPage(aBrowser, aParent, aShowEditUI) {
    var uri = aBrowser.currentURI;
    var itemId = PlacesUtils.getMostRecentBookmarkForURI(uri);
    if (itemId == -1) {
      
      
      
      
      
      
      var webNav = aBrowser.webNavigation;
      var url = webNav.currentURI;
      var title;
      var description;
      var charset;
      try {
        let isErrorPage = /^about:(neterror|certerror|blocked)/
                          .test(webNav.document.documentURI);
        title = isErrorPage ? PlacesUtils.history.getPageTitle(url)
                            : webNav.document.title;
        title = title || url.spec;
        description = PlacesUIUtils.getDescriptionFromDocument(webNav.document);
        charset = webNav.document.characterSet;
      }
      catch (e) { }

      if (aShowEditUI) {
        
        
        
        StarUI.beginBatch();
      }

      var parent = aParent != undefined ?
                   aParent : PlacesUtils.unfiledBookmarksFolderId;
      var descAnno = { name: PlacesUIUtils.DESCRIPTION_ANNO, value: description };
      var txn = new PlacesCreateBookmarkTransaction(uri, parent, 
                                                    PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                    title, null, [descAnno]);
      PlacesUtils.transactionManager.doTransaction(txn);
      itemId = txn.item.id;
      
      if (charset && !PrivateBrowsingUtils.isWindowPrivate(aBrowser.contentWindow))
        PlacesUtils.setCharsetForURI(uri, charset);
    }

    
    if (gURLBar)
      gURLBar.handleRevert();

    
    if (!aShowEditUI)
      return;

    
    
    
    
    if (BookmarkingUI.anchor) {
      StarUI.showEditBookmarkPopup(itemId, BookmarkingUI.anchor,
                                   "bottomcenter topright");
      return;
    }

    let pageProxyFavicon = document.getElementById("page-proxy-favicon");
    if (isElementVisible(pageProxyFavicon)) {
      StarUI.showEditBookmarkPopup(itemId, pageProxyFavicon,
                                   "bottomcenter topright");
    } else {
      StarUI.showEditBookmarkPopup(itemId, aBrowser, "overlap");
    }
  },

  


  bookmarkCurrentPage: function PCH_bookmarkCurrentPage(aShowEditUI, aParent) {
    this.bookmarkPage(gBrowser.selectedBrowser, aParent, aShowEditUI);
  },

  









  bookmarkLink: function PCH_bookmarkLink(aParent, aURL, aTitle) {
    var linkURI = makeURI(aURL);
    var itemId = PlacesUtils.getMostRecentBookmarkForURI(linkURI);
    if (itemId == -1) {
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "bookmark"
                                       , uri: linkURI
                                       , title: aTitle
                                       , hiddenRows: [ "description"
                                                     , "location"
                                                     , "loadInSidebar"
                                                     , "keyword" ]
                                       }, window);
    }
    else {
      PlacesUIUtils.showBookmarkDialog({ action: "edit"
                                       , type: "bookmark"
                                       , itemId: itemId
                                       }, window);
    }
  },

  




  get uniqueCurrentPages() {
    let uniquePages = {};
    let URIs = [];
    gBrowser.visibleTabs.forEach(function (tab) {
      let spec = tab.linkedBrowser.currentURI.spec;
      if (!tab.pinned && !(spec in uniquePages)) {
        uniquePages[spec] = null;
        URIs.push(tab.linkedBrowser.currentURI);
      }
    });
    return URIs;
  },

  



  bookmarkCurrentPages: function PCH_bookmarkCurrentPages() {
    let pages = this.uniqueCurrentPages;
    if (pages.length > 1) {
    PlacesUIUtils.showBookmarkDialog({ action: "add"
                                     , type: "folder"
                                     , URIList: pages
                                     , hiddenRows: [ "description" ]
                                     }, window);
    }
  },

  


  updateBookmarkAllTabsCommand:
  function PCH_updateBookmarkAllTabsCommand() {
    
    if (window.location.href != getBrowserURL())
      return;

    
    
    goSetCommandEnabled("Browser:BookmarkAllTabs",
                        this.uniqueCurrentPages.length >= 2);
  },

  








  addLiveBookmark: function PCH_addLiveBookmark(url, feedTitle, feedSubtitle) {
    var feedURI = makeURI(url);
    
    var doc = gBrowser.contentDocument;
    var title = (arguments.length > 1) ? feedTitle : doc.title;
 
    var description;
    if (arguments.length > 2)
      description = feedSubtitle;
    else
      description = PlacesUIUtils.getDescriptionFromDocument(doc);

    var toolbarIP = new InsertionPoint(PlacesUtils.toolbarFolderId, -1);
    PlacesUIUtils.showBookmarkDialog({ action: "add"
                                     , type: "livemark"
                                     , feedURI: feedURI
                                     , siteURI: gBrowser.currentURI
                                     , title: title
                                     , description: description
                                     , defaultInsertionPoint: toolbarIP
                                     , hiddenRows: [ "feedLocation"
                                                   , "siteLocation"
                                                   , "description" ]
                                     }, window);
  },

  






  showPlacesOrganizer: function PCH_showPlacesOrganizer(aLeftPaneRoot) {
    var organizer = Services.wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      
      openDialog("chrome://browser/content/places/places.xul", 
                 "", "chrome,toolbar=yes,dialog=no,resizable", aLeftPaneRoot);
    }
    else {
      organizer.PlacesOrganizer.selectLeftPaneContainerByHierarchy(aLeftPaneRoot);
      organizer.focus();
    }
  }
};




XPCOMUtils.defineLazyModuleGetter(this, "RecentlyClosedTabsAndWindowsMenuUtils",
  "resource:///modules/sessionstore/RecentlyClosedTabsAndWindowsMenuUtils.jsm");


function HistoryMenu(aPopupShowingEvent) {
  
  
  
  
  this.__proto__.__proto__ = PlacesMenu.prototype;
  PlacesMenu.call(this, aPopupShowingEvent,
                  "place:sort=4&maxResults=15");
}

HistoryMenu.prototype = {
  toggleRecentlyClosedTabs: function HM_toggleRecentlyClosedTabs() {
    
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedTabsMenu")[0];

    
    if (SessionStore.getClosedTabCount(window) == 0)
      undoMenu.setAttribute("disabled", true);
    else
      undoMenu.removeAttribute("disabled");
  },

  


  populateUndoSubmenu: function PHM_populateUndoSubmenu() {
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedTabsMenu")[0];
    var undoPopup = undoMenu.firstChild;

    
    while (undoPopup.hasChildNodes())
      undoPopup.removeChild(undoPopup.firstChild);

    
    if (SessionStore.getClosedTabCount(window) == 0) {
      undoMenu.setAttribute("disabled", true);
      return;
    }

    
    undoMenu.removeAttribute("disabled");

    
    let tabsFragment = RecentlyClosedTabsAndWindowsMenuUtils.getTabsFragment(window, "menuitem");
    undoPopup.appendChild(tabsFragment);
  },

  toggleRecentlyClosedWindows: function PHM_toggleRecentlyClosedWindows() {
    
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedWindowsMenu")[0];

    
    if (SessionStore.getClosedWindowCount() == 0)
      undoMenu.setAttribute("disabled", true);
    else
      undoMenu.removeAttribute("disabled");
  },

  


  populateUndoWindowSubmenu: function PHM_populateUndoWindowSubmenu() {
    let undoMenu = this._rootElt.getElementsByClassName("recentlyClosedWindowsMenu")[0];
    let undoPopup = undoMenu.firstChild;
    let menuLabelString = gNavigatorBundle.getString("menuUndoCloseWindowLabel");
    let menuLabelStringSingleTab =
      gNavigatorBundle.getString("menuUndoCloseWindowSingleTabLabel");

    
    while (undoPopup.hasChildNodes())
      undoPopup.removeChild(undoPopup.firstChild);

    
    if (SessionStore.getClosedWindowCount() == 0) {
      undoMenu.setAttribute("disabled", true);
      return;
    }

    
    undoMenu.removeAttribute("disabled");

    
    let windowsFragment = RecentlyClosedTabsAndWindowsMenuUtils.getWindowsFragment(window, "menuitem");
    undoPopup.appendChild(windowsFragment);
  },

  toggleTabsFromOtherComputers: function PHM_toggleTabsFromOtherComputers() {
    
#ifdef MOZ_SERVICES_SYNC
    
    
    let menuitem = this._rootElt.getElementsByClassName("syncTabsMenuItem")[0];
    if (!menuitem)
      return;

    if (!PlacesUIUtils.shouldShowTabsFromOtherComputersMenuitem()) {
      menuitem.setAttribute("hidden", true);
      return;
    }

    let enabled = PlacesUIUtils.shouldEnableTabsFromOtherComputersMenuitem();
    menuitem.setAttribute("disabled", !enabled);
    menuitem.setAttribute("hidden", false);
#endif
  },

  _onPopupShowing: function HM__onPopupShowing(aEvent) {
    PlacesMenu.prototype._onPopupShowing.apply(this, arguments);

    
    if (aEvent.target != aEvent.currentTarget)
      return;

    this.toggleRecentlyClosedTabs();
    this.toggleRecentlyClosedWindows();
    this.toggleTabsFromOtherComputers();
  },

  _onCommand: function HM__onCommand(aEvent) {
    let placesNode = aEvent.target._placesNode;
    if (placesNode) {
      if (!PrivateBrowsingUtils.isWindowPrivate(window))
        PlacesUIUtils.markPageAsTyped(placesNode.uri);
      openUILink(placesNode.uri, aEvent, { ignoreAlt: true });
    }
  }
};







var BookmarksEventHandler = {
  










  onClick: function BEH_onClick(aEvent, aView) {
    
#ifdef XP_MACOSX
    var modifKey = aEvent.metaKey || aEvent.shiftKey;
#else
    var modifKey = aEvent.ctrlKey || aEvent.shiftKey;
#endif
    if (aEvent.button == 2 || (aEvent.button == 0 && !modifKey))
      return;

    var target = aEvent.originalTarget;
    
    
    if (target.localName == "menu" || target.localName == "menuitem") {
      for (node = target.parentNode; node; node = node.parentNode) {
        if (node.localName == "menupopup")
          node.hidePopup();
        else if (node.localName != "menu" &&
                 node.localName != "splitmenu" &&
                 node.localName != "hbox" &&
                 node.localName != "vbox" )
          break;
      }
    }

    if (target._placesNode && PlacesUtils.nodeIsContainer(target._placesNode)) {
      
      
      
      if (target.localName == "menu" || target.localName == "toolbarbutton")
        PlacesUIUtils.openContainerNodeInTabs(target._placesNode, aEvent, aView);
    }
    else if (aEvent.button == 1) {
      
      this.onCommand(aEvent, aView);
    }
  },

  








  onCommand: function BEH_onCommand(aEvent, aView) {
    var target = aEvent.originalTarget;
    if (target._placesNode)
      PlacesUIUtils.openNodeWithEvent(target._placesNode, aEvent, aView);
  },

  fillInBHTooltip: function BEH_fillInBHTooltip(aDocument, aEvent) {
    var node;
    var cropped = false;
    var targetURI;

    if (aDocument.tooltipNode.localName == "treechildren") {
      var tree = aDocument.tooltipNode.parentNode;
      var row = {}, column = {};
      var tbo = tree.treeBoxObject;
      tbo.getCellAt(aEvent.clientX, aEvent.clientY, row, column, {});
      if (row.value == -1)
        return false;
      node = tree.view.nodeForTreeIndex(row.value);
      cropped = tbo.isCellCropped(row.value, column.value);
    }
    else {
      
      
      var tooltipNode = aDocument.tooltipNode;
      if (tooltipNode._placesNode)
        node = tooltipNode._placesNode;
      else {
        
        targetURI = tooltipNode.getAttribute("targetURI");
      }
    }

    if (!node && !targetURI)
      return false;

    
    var title = node ? node.title : tooltipNode.label;

    
    var url;
    if (targetURI || PlacesUtils.nodeIsURI(node))
      url = targetURI || node.uri;

    
    if (!cropped && !url)
      return false;

    var tooltipTitle = aDocument.getElementById("bhtTitleText");
    tooltipTitle.hidden = (!title || (title == url));
    if (!tooltipTitle.hidden)
      tooltipTitle.textContent = title;

    var tooltipUrl = aDocument.getElementById("bhtUrlText");
    tooltipUrl.hidden = !url;
    if (!tooltipUrl.hidden)
      tooltipUrl.value = url;

    
    return true;
  }
};






var PlacesMenuDNDHandler = {
  _springLoadDelayMs: 350,
  _closeDelayMs: 500,
  _loadTimer: null,
  _closeTimer: null,
  _closingTimerNode: null,

  




  onDragEnter: function PMDH_onDragEnter(event) {
    
    if (!this._isStaticContainer(event.target))
      return;

    
    
    if (this._closeTimer && this._closingTimerNode === event.currentTarget) {
      this._closeTimer.cancel();
      this._closingTimerNode = null;
      this._closeTimer = null;
    }

    PlacesControllerDragHelper.currentDropTarget = event.target;
    let popup = event.target.lastChild;
    if (this._loadTimer || popup.state === "showing" || popup.state === "open")
      return;

    this._loadTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._loadTimer.initWithCallback(() => {
      this._loadTimer = null;
      popup.setAttribute("autoopened", "true");
      popup.showPopup(popup);
    }, this._springLoadDelayMs, Ci.nsITimer.TYPE_ONE_SHOT);
    event.preventDefault();
    event.stopPropagation();
  },

  


  onDragLeave: function PMDH_onDragLeave(event) {
    
    if (event.relatedTarget === event.currentTarget ||
        (event.relatedTarget &&
         event.relatedTarget.parentNode === event.currentTarget))
      return;

    
    if (!this._isStaticContainer(event.target))
      return;

    PlacesControllerDragHelper.currentDropTarget = null;
    let popup = event.target.lastChild;

    if (this._loadTimer) {
      this._loadTimer.cancel();
      this._loadTimer = null;
    }
    this._closeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._closingTimerNode = event.currentTarget;
    this._closeTimer.initWithCallback(function() {
      this._closeTimer = null;
      this._closingTimerNode = null;
      let node = PlacesControllerDragHelper.currentDropTarget;
      let inHierarchy = false;
      while (node && !inHierarchy) {
        inHierarchy = node == event.target;
        node = node.parentNode;
      }
      if (!inHierarchy && popup && popup.hasAttribute("autoopened")) {
        popup.removeAttribute("autoopened");
        popup.hidePopup();
      }
    }, this._closeDelayMs, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  




  _isStaticContainer: function PMDH__isContainer(node) {
    let isMenu = node.localName == "menu" ||
                 (node.localName == "toolbarbutton" &&
                  (node.getAttribute("type") == "menu" ||
                   node.getAttribute("type") == "menu-button"));
    let isStatic = !("_placesNode" in node) && node.lastChild &&
                   node.lastChild.hasAttribute("placespopup") &&
                   !node.parentNode.hasAttribute("placespopup");
    return isMenu && isStatic;
  },

  




  onDragOver: function PMDH_onDragOver(event) {
    let ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId,
                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                Ci.nsITreeView.DROP_ON);
    if (ip && PlacesControllerDragHelper.canDrop(ip, event.dataTransfer))
      event.preventDefault();

    event.stopPropagation();
  },

  




  onDrop: function PMDH_onDrop(event) {
    
    let ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId,
                                PlacesUtils.bookmarks.DEFAULT_INDEX,
                                Ci.nsITreeView.DROP_ON);
    PlacesControllerDragHelper.onDrop(ip, event.dataTransfer);
    PlacesControllerDragHelper.currentDropTarget = null;
    event.stopPropagation();
  }
};








let PlacesToolbarHelper = {
  _place: "place:folder=TOOLBAR",

  get _viewElt() {
    return document.getElementById("PlacesToolbar");
  },

  get _placeholder() {
    return document.getElementById("bookmarks-toolbar-placeholder");
  },

  init: function PTH_init(forceToolbarOverflowCheck) {
    let viewElt = this._viewElt;
    if (!viewElt || viewElt._placesView)
      return;

    
    
    
    
    
    
    let toolbar = this._getParentToolbar(viewElt);
    if (!toolbar || toolbar.collapsed || this._isCustomizing ||
        getComputedStyle(toolbar, "").display == "none")
      return;

    new PlacesToolbar(this._place);
    if (forceToolbarOverflowCheck) {
      viewElt._placesView.updateOverflowStatus();
    }
    this.customizeChange();
  },

  customizeStart: function PTH_customizeStart() {
    try {
      let viewElt = this._viewElt;
      if (viewElt && viewElt._placesView)
        viewElt._placesView.uninit();
    } finally {
      this._isCustomizing = true;
    }
    this._shouldWrap = this._getShouldWrap();
  },

  customizeChange: function PTH_customizeChange() {
    let placeholder = this._placeholder;
    if (!placeholder) {
      return;
    }
    let shouldWrapNow = this._getShouldWrap();
    if (this._shouldWrap != shouldWrapNow) {
      if (shouldWrapNow) {
        placeholder.setAttribute("wrap", "true");
      } else {
        placeholder.removeAttribute("wrap");
      }
      placeholder.classList.toggle("toolbarbutton-1", shouldWrapNow);
      this._shouldWrap = shouldWrapNow;
    }
  },

  customizeDone: function PTH_customizeDone() {
    this._isCustomizing = false;
    this.init(true);
  },

  _getShouldWrap: function PTH_getShouldWrap() {
    let placement = CustomizableUI.getPlacementOfWidget("personal-bookmarks");
    let area = placement && placement.area;
    let areaType = area && CustomizableUI.getAreaType(area);
    return !area || CustomizableUI.TYPE_MENU_PANEL == areaType;
  },

  onPlaceholderCommand: function () {
    let widgetGroup = CustomizableUI.getWidget("personal-bookmarks");
    let widget = widgetGroup.forWindow(window);
    if (widget.overflowed ||
        widgetGroup.areaType == CustomizableUI.TYPE_MENU_PANEL) {
      PlacesCommandHook.showPlacesOrganizer("BookmarksToolbar");
    }
  },

  _getParentToolbar: function(element) {
    while (element) {
      if (element.localName == "toolbar") {
        return element;
      }
      element = element.parentNode;
    }
    return null;
  }
};








let BookmarkingUI = {
  get button() {
    delete this.button;
    let widgetGroup = CustomizableUI.getWidget("bookmarks-menu-button");
    return this.button = widgetGroup.forWindow(window).node;
  },

  

  get star() {
    return document.getAnonymousElementByAttribute(this.button, "anonid",
                                                   "button");
  },

  get anchor() {
    if (!this._shouldUpdateStarState()) {
      return null;
    }
    let widget = CustomizableUI.getWidget("bookmarks-menu-button")
                               .forWindow(window);
    if (widget.overflowed)
      return widget.anchor;

    let star = this.star;
    return star ? document.getAnonymousElementByAttribute(star, "class",
                                                          "toolbarbutton-icon")
                : null;
  },

  get notifier() {
    delete this.notifier;
    return this.notifier = document.getElementById("bookmarked-notification-anchor");
  },

  get broadcaster() {
    delete this.broadcaster;
    let broadcaster = document.getElementById("bookmarkThisPageBroadcaster");
    return this.broadcaster = broadcaster;
  },

  STATUS_UPDATING: -1,
  STATUS_UNSTARRED: 0,
  STATUS_STARRED: 1,
  get status() {
    if (!this._shouldUpdateStarState()) {
      return this.STATUS_UNSTARRED;
    }
    if (this._pendingStmt)
      return this.STATUS_UPDATING;
    return this.button.hasAttribute("starred") ? this.STATUS_STARRED
                                               : this.STATUS_UNSTARRED;
  },

  get _starredTooltip()
  {
    delete this._starredTooltip;
    return this._starredTooltip =
      gNavigatorBundle.getString("starButtonOn.tooltip");
  },

  get _unstarredTooltip()
  {
    delete this._unstarredTooltip;
    return this._unstarredTooltip =
      gNavigatorBundle.getString("starButtonOff.tooltip");
  },

  



  _currentAreaType: null,
  _shouldUpdateStarState: function() {
    return this._currentAreaType == CustomizableUI.TYPE_TOOLBAR;
  },

  





  _popupNeedsUpdate: true,
  onToolbarVisibilityChange: function BUI_onToolbarVisibilityChange() {
    this._popupNeedsUpdate = true;
  },

  onPopupShowing: function BUI_onPopupShowing(event) {
    
    if (event.target != event.currentTarget)
      return;

    
    
    
    if (this._currentAreaType == CustomizableUI.TYPE_MENU_PANEL) {
      this._showSubview();
      event.preventDefault();
      event.stopPropagation();
      return;
    }

    let widget = CustomizableUI.getWidget("bookmarks-menu-button")
                               .forWindow(window);
    if (widget.overflowed) {
      
      event.preventDefault();
      widget.node.removeAttribute("closemenu");
      PlacesCommandHook.showPlacesOrganizer("BookmarksMenu");
      return;
    }

    if (!this._popupNeedsUpdate)
      return;
    this._popupNeedsUpdate = false;

    let popup = event.target;
    let getPlacesAnonymousElement =
      aAnonId => document.getAnonymousElementByAttribute(popup.parentNode,
                                                         "placesanonid",
                                                         aAnonId);

    let viewToolbarMenuitem = getPlacesAnonymousElement("view-toolbar");
    if (viewToolbarMenuitem) {
      
      let personalToolbar = document.getElementById("PersonalToolbar");
      viewToolbarMenuitem.setAttribute("checked", !personalToolbar.collapsed);
    }
  },

  attachPlacesView: function(event, node) {
    
    if (node.parentNode._placesView)
      return;

    new PlacesMenu(event, "place:folder=BOOKMARKS_MENU", {
      extraClasses: {
        mainLevel: "subviewbutton"
      },
      insertionPoint: ".panel-subview-footer"
    });
  },

  


  onPageProxyStateChanged: function BUI_onPageProxyStateChanged(aState) {
    if (!this._shouldUpdateStarState() || !this.star) {
      return;
    }

    if (aState == "invalid") {
      this.star.setAttribute("disabled", "true");
      this.button.removeAttribute("starred");
      this.button.setAttribute("buttontooltiptext", "");
    }
    else {
      this.star.removeAttribute("disabled");
      this._updateStar();
    }
    this._updateToolbarStyle();
  },

  _updateCustomizationState: function BUI__updateCustomizationState() {
    let placement = CustomizableUI.getPlacementOfWidget("bookmarks-menu-button");
    this._currentAreaType = placement && CustomizableUI.getAreaType(placement.area);
  },

  _updateToolbarStyle: function BUI__updateToolbarStyle() {
    let onPersonalToolbar = false;
    if (this._currentAreaType == CustomizableUI.TYPE_TOOLBAR) {
      let personalToolbar = document.getElementById("PersonalToolbar");
      onPersonalToolbar = this.button.parentNode == personalToolbar ||
                          this.button.parentNode.parentNode == personalToolbar;
    }

    if (onPersonalToolbar) {
      this.button.classList.add("bookmark-item");
      this.button.classList.remove("toolbarbutton-1");
    }
    else {
      this.button.classList.remove("bookmark-item");
      this.button.classList.add("toolbarbutton-1");
    }
  },

  _uninitView: function BUI__uninitView() {
    
    
    
    if (this.button._placesView)
      this.button._placesView.uninit();
  },

  onCustomizeStart: function BUI_customizeStart(aWindow) {
    if (aWindow == window) {
      this._uninitView();
      this._isCustomizing = true;
    }
  },

  onWidgetAdded: function BUI_widgetAdded(aWidgetId) {
    if (aWidgetId != "bookmarks-menu-button") {
      return;
    }

    let usedToUpdateStarState = this._shouldUpdateStarState();
    this._updateCustomizationState();
    if (!usedToUpdateStarState && this._shouldUpdateStarState()) {
      this.updateStarState();
    } else if (usedToUpdateStarState && !this._shouldUpdateStarState()) {
      this._updateStar();
    }
    
    
    if (!this._isCustomizing) {
      this._uninitView();
    }
    this._updateToolbarStyle();
  },

  onWidgetRemoved: function BUI_widgetRemoved(aWidgetId) {
    if (aWidgetId != "bookmarks-menu-button") {
      return;
    }
    
    
    if (!this._isCustomizing) {
      this._uninitView();
    }
    this._updateCustomizationState();
    this._updateToolbarStyle();
  },

  onCustomizeEnd: function BUI_customizeEnd(aWindow) {
    if (aWindow == window) {
      this._isCustomizing = false;
      this.onToolbarVisibilityChange();
      this._updateToolbarStyle();
    }
  },

  init: function() {
    CustomizableUI.addListener(this);
    this._updateCustomizationState();
  },

  _hasBookmarksObserver: false,
  _itemIds: [],
  uninit: function BUI_uninit() {
    this._updateBookmarkPageMenuItem(true);
    CustomizableUI.removeListener(this);

    this._uninitView();

    if (this._hasBookmarksObserver) {
      PlacesUtils.removeLazyBookmarkObserver(this);
    }

    if (this._pendingStmt) {
      this._pendingStmt.cancel();
      delete this._pendingStmt;
    }
  },

  onLocationChange: function BUI_onLocationChange() {
    if (this._uri && gBrowser.currentURI.equals(this._uri)) {
      return;
    }
    this.updateStarState();
  },

  updateStarState: function BUI_updateStarState() {
    
    this._uri = gBrowser.currentURI;
    this._itemIds = [];

    if (this._pendingStmt) {
      this._pendingStmt.cancel();
      delete this._pendingStmt;
    }

    
    if (isBlankPageURL(this._uri.spec)) {
      return;
    }

    this._pendingStmt = PlacesUtils.asyncGetBookmarkIds(this._uri, function (aItemIds, aURI) {
      
      if (!aURI.equals(this._uri)) {
        Components.utils.reportError("BookmarkingUI did not receive current URI");
        return;
      }

      
      
      
      this._itemIds = this._itemIds.filter(
        function (id) aItemIds.indexOf(id) == -1
      ).concat(aItemIds);

      this._updateStar();

      
      if (!this._hasBookmarksObserver) {
        try {
          PlacesUtils.addLazyBookmarkObserver(this);
          this._hasBookmarksObserver = true;
        } catch(ex) {
          Components.utils.reportError("BookmarkingUI failed adding a bookmarks observer: " + ex);
        }
      }

      delete this._pendingStmt;
    }, this);
  },

  _updateStar: function BUI__updateStar() {
    if (!this._shouldUpdateStarState()) {
      if (this.button.hasAttribute("starred")) {
        this.button.removeAttribute("starred");
        this.button.removeAttribute("buttontooltiptext");
      }
      return;
    }

    if (this._itemIds.length > 0) {
      this.button.setAttribute("starred", "true");
      this.button.setAttribute("buttontooltiptext", this._starredTooltip);
    }
    else {
      this.button.removeAttribute("starred");
      this.button.setAttribute("buttontooltiptext", this._unstarredTooltip);
    }
  },

  



  _updateBookmarkPageMenuItem: function BUI__updateBookmarkPageMenuItem(forceReset) {
    let isStarred = !forceReset && this._itemIds.length > 0;
    let label = isStarred ? "editlabel" : "bookmarklabel";
    this.broadcaster.setAttribute("label", this.broadcaster.getAttribute(label));
  },

  onMainMenuPopupShowing: function BUI_onMainMenuPopupShowing(event) {
    this._updateBookmarkPageMenuItem();
    PlacesCommandHook.updateBookmarkAllTabsCommand();
  },

  _showBookmarkedNotification: function BUI_showBookmarkedNotification() {

    if (this._notificationTimeout) {
      clearTimeout(this._notificationTimeout);
    }

    if (this.notifier.style.transform == '') {
      let buttonRect = this.button.getBoundingClientRect();
      let notifierRect = this.notifier.getBoundingClientRect();
      let topDiff = buttonRect.top - notifierRect.top;
      let leftDiff = buttonRect.left - notifierRect.left;
      let heightDiff = buttonRect.height - notifierRect.height;
      let widthDiff = buttonRect.width - notifierRect.width;
      let translateX = (leftDiff + .5 * widthDiff) + "px";
      let translateY = (topDiff + .5 * heightDiff) + "px";
      this.notifier.style.transform = "translate(" +  translateX + ", " + translateY + ")";
    }

    let isInBookmarksToolbar = this.button.classList.contains("bookmark-item");
    if (isInBookmarksToolbar)
      this.notifier.setAttribute("in-bookmarks-toolbar", true);

    let isInOverflowPanel = this.button.classList.contains("overflowedItem");
    if (!isInOverflowPanel) {
      this.notifier.setAttribute("notification", "finish");
      this.button.setAttribute("notification", "finish");
    }

    this._notificationTimeout = setTimeout( () => {
      this.notifier.removeAttribute("notification");
      this.notifier.removeAttribute("in-bookmarks-toolbar");
      this.button.removeAttribute("notification");
      this.notifier.style.transform = '';
    }, 1000);
  },

  _showSubview: function() {
    let view = document.getElementById("PanelUI-bookmarks");
    view.addEventListener("ViewShowing", this);
    view.addEventListener("ViewHiding", this);
    let anchor = document.getElementById("bookmarks-menu-button");
    anchor.setAttribute("closemenu", "none");
    PanelUI.showSubView("PanelUI-bookmarks", anchor,
                        CustomizableUI.AREA_PANEL);
  },

  onCommand: function BUI_onCommand(aEvent) {
    if (aEvent.target != aEvent.currentTarget) {
      return;
    }

    
    let isBookmarked = this._itemIds.length > 0;

    if (this._currentAreaType == CustomizableUI.TYPE_MENU_PANEL) {
      this._showSubview();
      return;
    }
    let widget = CustomizableUI.getWidget("bookmarks-menu-button")
                               .forWindow(window);
    if (widget.overflowed) {
      
      
      if (isBookmarked)
        widget.node.removeAttribute("closemenu");
      else
        widget.node.setAttribute("closemenu", "none");
    }

    
    if (!this._pendingStmt) {
      if (!isBookmarked)
        this._showBookmarkedNotification();
      PlacesCommandHook.bookmarkCurrentPage(isBookmarked);
    }
  },

  handleEvent: function BUI_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "ViewShowing":
        this.onPanelMenuViewShowing(aEvent);
        break;
      case "ViewHiding":
        this.onPanelMenuViewHiding(aEvent);
        break;
    }
  },

  onPanelMenuViewShowing: function BUI_onViewShowing(aEvent) {
    this._updateBookmarkPageMenuItem();
    
    let viewToolbar = document.getElementById("panelMenu_viewBookmarksToolbar");
    let personalToolbar = document.getElementById("PersonalToolbar");
    if (personalToolbar.collapsed)
      viewToolbar.removeAttribute("checked");
    else
      viewToolbar.setAttribute("checked", "true");
    
    this._panelMenuView = new PlacesPanelMenuView("place:folder=BOOKMARKS_MENU",
                                                  "panelMenu_bookmarksMenu",
                                                  "panelMenu_bookmarksMenu", {
                                                    extraClasses: {
                                                      mainLevel: "subviewbutton"
                                                    }
                                                  });
    aEvent.target.removeEventListener("ViewShowing", this);
  },

  onPanelMenuViewHiding: function BUI_onViewHiding(aEvent) {
    this._panelMenuView.uninit();
    delete this._panelMenuView;
    aEvent.target.removeEventListener("ViewHiding", this);
  },

  onPanelMenuViewCommand: function BUI_onPanelMenuViewCommand(aEvent, aView) {
    let target = aEvent.originalTarget;
    if (!target._placesNode)
      return;
    if (PlacesUtils.nodeIsContainer(target._placesNode))
      PlacesCommandHook.showPlacesOrganizer([ "BookmarksMenu", target._placesNode.itemId ]);
    else
      PlacesUIUtils.openNodeWithEvent(target._placesNode, aEvent, aView);
    PanelUI.hide();
  },

  
  onItemAdded: function BUI_onItemAdded(aItemId, aParentId, aIndex, aItemType,
                                        aURI) {
    if (aURI && aURI.equals(this._uri)) {
      
      if (this._itemIds.indexOf(aItemId) == -1) {
        this._itemIds.push(aItemId);
        
        if (this._itemIds.length == 1) {
          this._updateStar();
        }
      }
    }
  },

  onItemRemoved: function BUI_onItemRemoved(aItemId) {
    let index = this._itemIds.indexOf(aItemId);
    
    if (index != -1) {
      this._itemIds.splice(index, 1);
      
      if (this._itemIds.length == 0) {
        this._updateStar();
      }
    }
  },

  onItemChanged: function BUI_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aNewValue) {
    if (aProperty == "uri") {
      let index = this._itemIds.indexOf(aItemId);
      
      
      if (index != -1 && aNewValue != this._uri.spec) {
        this._itemIds.splice(index, 1);
        
        if (this._itemIds.length == 0) {
          this._updateStar();
        }
      }
      
      else if (index == -1 && aNewValue == this._uri.spec) {
        this._itemIds.push(aItemId);
        
        if (this._itemIds.length == 1) {
          this._updateStar();
        }
      }
    }
  },

  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onBeforeItemRemoved: function () {},
  onItemVisited: function () {},
  onItemMoved: function () {},

  
  _starButtonLabel: null,
  get _starButtonOverflowedLabel() {
    delete this._starButtonOverflowedLabel;
    return this._starButtonOverflowedLabel =
      gNavigatorBundle.getString("starButtonOverflowed.label");
  },
  get _starButtonOverflowedStarredLabel() {
    delete this._starButtonOverflowedStarredLabel;
    return this._starButtonOverflowedStarredLabel =
      gNavigatorBundle.getString("starButtonOverflowedStarred.label");
  },
  onWidgetOverflow: function(aNode, aContainer) {
    let win = aNode.ownerDocument.defaultView;
    if (aNode.id != "bookmarks-menu-button" || win != window)
      return;

    let currentLabel = aNode.getAttribute("label");
    if (!this._starButtonLabel)
      this._starButtonLabel = currentLabel;

    if (currentLabel == this._starButtonLabel) {
      let desiredLabel = this._itemIds.length > 0 ? this._starButtonOverflowedStarredLabel
                                                 : this._starButtonOverflowedLabel;
      aNode.setAttribute("label", desiredLabel);
    }
  },

  onWidgetUnderflow: function(aNode, aContainer) {
    let win = aNode.ownerDocument.defaultView;
    if (aNode.id != "bookmarks-menu-button" || win != window)
      return;

    
    
    this._uninitView();

    if (aNode.getAttribute("label") != this._starButtonLabel)
      aNode.setAttribute("label", this._starButtonLabel);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver
  ])
};
