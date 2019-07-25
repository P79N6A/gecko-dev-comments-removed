# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Places Browser Integration
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com>
#   Annie Sullivan <annie.sullivan@gmail.com>
#   Joe Hughes <joe@retrovirus.com>
#   Asaf Romano <mano@mozilla.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#   Marco Bonardo <mak77@bonardo.net>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****


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
            PlacesUIUtils.ptm.endBatch();
            this._batching = false;
          }

          switch (this._actionOnHide) {
            case "cancel": {
              PlacesUIUtils.ptm.undoTransaction();
              break;
            }
            case "remove": {
              
              
              PlacesUIUtils.ptm.beginBatch();
              let itemIds = PlacesUtils.getBookmarksForURI(this._uriForRemoval);
              for (let i = 0; i < itemIds.length; i++) {
                let txn = PlacesUIUtils.ptm.removeItem(itemIds[i]);
                PlacesUIUtils.ptm.doTransaction(txn);
              }
              PlacesUIUtils.ptm.endBatch();
              break;
            }
          }
          this._actionOnHide = "";
        }
        break;
      case "keypress":
        if (aEvent.getPreventDefault()) {
          
          break;
        }
        switch (aEvent.keyCode) {
          case KeyEvent.DOM_VK_ESCAPE:
            if (!this._element("editBookmarkPanelContent").hidden)
              this.cancelButtonOnCommand();
            break;
          case KeyEvent.DOM_VK_RETURN:
            if (aEvent.target.className == "expander-up" ||
                aEvent.target.className == "expander-down" ||
                aEvent.target.id == "editBMPanel_newFolderButton") {
              
              
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

    var loadObserver = {
      _self: this,
      _itemId: aItemId,
      _anchorElement: aAnchorElement,
      _position: aPosition,
      observe: function (aSubject, aTopic, aData) {
        
        retrieveToolbarIconsizesFromTheme();

        this._self._overlayLoading = false;
        this._self._overlayLoaded = true;
        this._self._doShowEditBookmarkPanel(this._itemId, this._anchorElement,
                                            this._position);
      }
    };
    this._overlayLoading = true;
    document.loadOverlay("chrome://browser/content/places/editBookmarkOverlay.xul",
                         loadObserver);
  },

  _doShowEditBookmarkPanel:
  function SU__doShowEditBookmarkPanel(aItemId, aAnchorElement, aPosition) {
    if (this.panel.state != "closed")
      return;

    this._blockCommands(); 

    
    
    var rows = this._element("editBookmarkPanelGrid").lastChild;
    var header = this._element("editBookmarkPanelHeader");
    rows.insertBefore(header, rows.firstChild);
    header.hidden = false;

    
    
    
    
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

    
    this.panel.popupBoxObject
        .setConsumeRollupEvent(Ci.nsIPopupBoxObject.ROLLUP_CONSUME);
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

  editButtonCommand: function SU_editButtonCommand() {
    this.showEditBookmarkPopup();
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
      PlacesUIUtils.ptm.beginBatch();
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
        title = webNav.document.title || url.spec;
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
      var txn = PlacesUIUtils.ptm.createItem(uri, parent, -1,
                                             title, null, [descAnno]);
      PlacesUIUtils.ptm.doTransaction(txn);
      
      if (charset)
        PlacesUtils.history.setCharsetForURI(uri, charset);
      itemId = PlacesUtils.getMostRecentBookmarkForURI(uri);
    }

    
    if (gURLBar)
      gURLBar.handleRevert();

    
    
    if (aBrowser.contentWindow == window.content) {
      var starIcon = aBrowser.ownerDocument.getElementById("star-button");
      if (starIcon && isElementVisible(starIcon)) {
        if (aShowEditUI)
          StarUI.showEditBookmarkPopup(itemId, starIcon, "bottomcenter topright");
        return;
      }
    }

    StarUI.showEditBookmarkPopup(itemId, aBrowser, "overlap");
  },

  


  bookmarkCurrentPage: function PCH_bookmarkCurrentPage(aShowEditUI, aParent) {
    this.bookmarkPage(gBrowser.selectedBrowser, aParent, aShowEditUI);
  },

  









  bookmarkLink: function PCH_bookmarkLink(aParent, aURL, aTitle) {
    var linkURI = makeURI(aURL);
    var itemId = PlacesUtils.getMostRecentBookmarkForURI(linkURI);
    if (itemId == -1)
      PlacesUIUtils.showMinimalAddBookmarkUI(linkURI, aTitle);
    else {
      PlacesUIUtils.showItemProperties(itemId,
                                       PlacesUtils.bookmarks.TYPE_BOOKMARK);
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
      PlacesUIUtils.showMinimalAddMultiBookmarkUI(pages);
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

    var toolbarIP =
      new InsertionPoint(PlacesUtils.bookmarks.toolbarFolder, -1);
    PlacesUIUtils.showMinimalAddLivemarkUI(feedURI, gBrowser.currentURI,
                                           title, description, toolbarIP, true);
  },

  






  showPlacesOrganizer: function PCH_showPlacesOrganizer(aLeftPaneRoot) {
    var organizer = Services.wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      
      openDialog("chrome:
                 "", "chrome,toolbar=yes,dialog=no,resizable", aLeftPaneRoot);
    }
    else {
      organizer.PlacesOrganizer.selectLeftPaneQuery(aLeftPaneRoot);
      organizer.focus();
    }
  }
};


function HistoryMenu(aPopupShowingEvent) {
  
  
  
  
  this.__proto__.__proto__ = PlacesMenu.prototype;
  XPCOMUtils.defineLazyServiceGetter(this, "_ss",
                                     "@mozilla.org/browser/sessionstore;1",
                                     "nsISessionStore");
  PlacesMenu.call(this, aPopupShowingEvent,
                  "place:redirectsMode=2&sort=4&maxResults=15");
}

HistoryMenu.prototype = {
  toggleRecentlyClosedTabs: function HM_toggleRecentlyClosedTabs() {
    
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedTabsMenu")[0];

    
    if (this._ss.getClosedTabCount(window) == 0)
      undoMenu.setAttribute("disabled", true);
    else
      undoMenu.removeAttribute("disabled");
  },

  





  _undoCloseMiddleClick: function PHM__undoCloseMiddleClick(aEvent) {
    if (aEvent.button != 1)
      return;

    undoCloseTab(aEvent.originalTarget.value);
    gBrowser.moveTabToEnd();
  },

  


  populateUndoSubmenu: function PHM_populateUndoSubmenu() {
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedTabsMenu")[0];
    var undoPopup = undoMenu.firstChild;

    
    while (undoPopup.hasChildNodes())
      undoPopup.removeChild(undoPopup.firstChild);

    
    if (this._ss.getClosedTabCount(window) == 0) {
      undoMenu.setAttribute("disabled", true);
      return;
    }

    
    undoMenu.removeAttribute("disabled");

    
    var undoItems = eval("(" + this._ss.getClosedTabData(window) + ")");
    for (var i = 0; i < undoItems.length; i++) {
      var m = document.createElement("menuitem");
      m.setAttribute("label", undoItems[i].title);
      if (undoItems[i].image) {
        let iconURL = undoItems[i].image;
        
        if (/^https?:/.test(iconURL))
          iconURL = "moz-anno:favicon:" + iconURL;
        m.setAttribute("image", iconURL);
      }
      m.setAttribute("class", "menuitem-iconic bookmark-item menuitem-with-favicon");
      m.setAttribute("value", i);
      m.setAttribute("oncommand", "undoCloseTab(" + i + ");");

      
      
      
      let tabData = undoItems[i].state;
      let activeIndex = (tabData.index || tabData.entries.length) - 1;
      if (activeIndex >= 0 && tabData.entries[activeIndex])
        m.setAttribute("targetURI", tabData.entries[activeIndex].url);

      m.addEventListener("click", this._undoCloseMiddleClick, false);
      if (i == 0)
        m.setAttribute("key", "key_undoCloseTab");
      undoPopup.appendChild(m);
    }

    
    var strings = gNavigatorBundle;
    undoPopup.appendChild(document.createElement("menuseparator"));
    m = undoPopup.appendChild(document.createElement("menuitem"));
    m.id = "menu_restoreAllTabs";
    m.setAttribute("label", strings.getString("menuRestoreAllTabs.label"));
    m.addEventListener("command", function() {
      for (var i = 0; i < undoItems.length; i++)
        undoCloseTab();
    }, false);
  },

  toggleRecentlyClosedWindows: function PHM_toggleRecentlyClosedWindows() {
    
    var undoMenu = this._rootElt.getElementsByClassName("recentlyClosedWindowsMenu")[0];

    
    if (this._ss.getClosedWindowCount() == 0)
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

    
    if (this._ss.getClosedWindowCount() == 0) {
      undoMenu.setAttribute("disabled", true);
      return;
    }

    
    undoMenu.removeAttribute("disabled");

    
    let undoItems = JSON.parse(this._ss.getClosedWindowData());
    for (let i = 0; i < undoItems.length; i++) {
      let undoItem = undoItems[i];
      let otherTabsCount = undoItem.tabs.length - 1;
      let label = (otherTabsCount == 0) ? menuLabelStringSingleTab
                                        : PluralForm.get(otherTabsCount, menuLabelString);
      let menuLabel = label.replace("#1", undoItem.title)
                           .replace("#2", otherTabsCount);
      let m = document.createElement("menuitem");
      m.setAttribute("label", menuLabel);
      let selectedTab = undoItem.tabs[undoItem.selected - 1];
      if (selectedTab.attributes.image) {
        let iconURL = selectedTab.attributes.image;
        
        if (/^https?:/.test(iconURL))
          iconURL = "moz-anno:favicon:" + iconURL;
        m.setAttribute("image", iconURL);
      }
      m.setAttribute("class", "menuitem-iconic bookmark-item menuitem-with-favicon");
      m.setAttribute("oncommand", "undoCloseWindow(" + i + ");");

      
      
      let activeIndex = (selectedTab.index || selectedTab.entries.length) - 1;
      if (activeIndex >= 0 && selectedTab.entries[activeIndex])
        m.setAttribute("targetURI", selectedTab.entries[activeIndex].url);

      if (i == 0)
        m.setAttribute("key", "key_undoCloseWindow");
      undoPopup.appendChild(m);
    }

    
    undoPopup.appendChild(document.createElement("menuseparator"));
    let m = undoPopup.appendChild(document.createElement("menuitem"));
    m.id = "menu_restoreAllWindows";
    m.setAttribute("label", gNavigatorBundle.getString("menuRestoreAllWindows.label"));
    m.setAttribute("oncommand",
      "for (var i = 0; i < " + undoItems.length + "; i++) undoCloseWindow();");
  },

  toggleTabsFromOtherComputers: function PHM_toggleTabsFromOtherComputers() {
    
#ifdef MOZ_SERVICES_SYNC
    
    let menuitem = document.getElementById("sync-tabs-menuitem");

    
    if (Weave.Status.checkSetup() == Weave.CLIENT_NOT_CONFIGURED ||
        Weave.Svc.Prefs.get("firstSync", "") == "notReady") {
      menuitem.setAttribute("hidden", true);
      return;
    }

    
    
    let enabled = Weave.Service.isLoggedIn && Weave.Engines.get("tabs") &&
                  Weave.Engines.get("tabs").enabled;
    menuitem.setAttribute("disabled", !enabled);
    menuitem.setAttribute("hidden", false);
#endif
  },

  toggleRestoreLastSession: function PHM_toggleRestoreLastSession() {
    let restoreItem = this._rootElt.getElementsByClassName("restoreLastSession")[0];

    if (this._ss.canRestoreLastSession)
      restoreItem.removeAttribute("disabled");
    else
      restoreItem.setAttribute("disabled", true);
  },

  _onPopupShowing: function HM__onPopupShowing(aEvent) {
    PlacesMenu.prototype._onPopupShowing.apply(this, arguments);

    
    if (aEvent.target != aEvent.currentTarget)
      return;

    this.toggleRecentlyClosedTabs();
    this.toggleRecentlyClosedWindows();
    this.toggleTabsFromOtherComputers();
    this.toggleRestoreLastSession();
  },

  _onCommand: function HM__onCommand(aEvent) {
    let placesNode = aEvent.target._placesNode;
    if (placesNode) {
      PlacesUIUtils.markPageAsTyped(placesNode.uri);
      openUILink(placesNode.uri, aEvent, false, true);
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
  _springLoadDelay: 350, 
  _loadTimer: null,

  




  onDragEnter: function PMDH_onDragEnter(event) {
    
    if (!this._isStaticContainer(event.target))
      return;

    this._loadTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._loadTimer.initWithCallback(function() {
      PlacesMenuDNDHandler._loadTimer = null;
      event.target.lastChild.setAttribute("autoopened", "true");
      event.target.lastChild.showPopup(event.target.lastChild);
    }, this._springLoadDelay, Ci.nsITimer.TYPE_ONE_SHOT);
    event.preventDefault();
    event.stopPropagation();
  },

  




  onDragExit: function PMDH_onDragExit(event) {
    
    if (!this._isStaticContainer(event.target))
      return;

    if (this._loadTimer) {
      this._loadTimer.cancel();
      this._loadTimer = null;
    }
    let closeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    closeTimer.initWithCallback(function() {
      let node = PlacesControllerDragHelper.currentDropTarget;
      let inHierarchy = false;
      while (node && !inHierarchy) {
        inHierarchy = node == event.target;
        node = node.parentNode;
      }
      if (!inHierarchy && event.target.lastChild &&
          event.target.lastChild.hasAttribute("autoopened")) {
        event.target.lastChild.removeAttribute("autoopened");
        event.target.lastChild.hidePopup();
      }
    }, this._springLoadDelay, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  




  _isStaticContainer: function PMDH__isContainer(node) {
    let isMenu = node.localName == "menu" ||
                 (node.localName == "toolbarbutton" &&
                  node.getAttribute("type") == "menu");
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
    event.stopPropagation();
  }
};


var PlacesStarButton = {
  _hasBookmarksObserver: false,
  uninit: function PSB_uninit()
  {
    if (this._hasBookmarksObserver) {
      PlacesUtils.bookmarks.removeObserver(this);
    }
    if (this._pendingStmt) {
      this._pendingStmt.cancel();
      delete this._pendingStmt;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver
  ]),

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

  updateState: function PSB_updateState()
  {
    this._starIcon = document.getElementById("star-button");
    if (!this._starIcon || (this._uri && gBrowser.currentURI.equals(this._uri))) {
      return;
    }

    
    this._uri = gBrowser.currentURI;
    this._itemIds = [];

    if (this._pendingStmt) {
      this._pendingStmt.cancel();
      delete this._pendingStmt;
    }

    
    if (this._uri.spec == "about:blank") {
      return;
    }

    this._pendingStmt = PlacesUtils.asyncGetBookmarkIds(this._uri, function (aItemIds, aURI) {
      
      if (!aURI.equals(this._uri)) {
        Components.utils.reportError("PlacesStarButton did not receive current URI");
        return;
      }

      
      
      
      this._itemIds = this._itemIds.filter(
        function (id) aItemIds.indexOf(id) == -1
      ).concat(aItemIds);
      this._updateStateInternal();

      
      if (!this._hasBookmarksObserver) {
        try {
          PlacesUtils.bookmarks.addObserver(this, false);
          this._hasBookmarksObserver = true;
        } catch(ex) {
          Components.utils.reportError("PlacesStarButton failed adding a bookmarks observer: " + ex);
        }
      }

      delete this._pendingStmt;
    }, this);
  },

  _updateStateInternal: function PSB__updateStateInternal()
  {
    if (!this._starIcon) {
      return;
    }

    if (this._itemIds.length > 0) {
      this._starIcon.setAttribute("starred", "true");
      this._starIcon.setAttribute("tooltiptext", this._starredTooltip);
    }
    else {
      this._starIcon.removeAttribute("starred");
      this._starIcon.setAttribute("tooltiptext", this._unstarredTooltip);
    }
  },

  onClick: function PSB_onClick(aEvent)
  {
    
    if (aEvent.button == 0 && !this._pendingStmt) {
      PlacesCommandHook.bookmarkCurrentPage(this._itemIds.length > 0);
    }
    
    aEvent.stopPropagation();
  },

  
  onItemAdded:
  function PSB_onItemAdded(aItemId, aFolder, aIndex, aItemType, aURI)
  {
    if (!this._starIcon) {
      return;
    }

    if (aURI && aURI.equals(this._uri)) {
      
      if (this._itemIds.indexOf(aItemId) == -1) {
        this._itemIds.push(aItemId);
        this._updateStateInternal();
      }
    }
  },

  onItemRemoved:
  function PSB_onItemRemoved(aItemId, aFolder, aIndex, aItemType)
  {
    if (!this._starIcon) {
      return;
    }

    let index = this._itemIds.indexOf(aItemId);
    
    if (index != -1) {
      this._itemIds.splice(index, 1);
      this._updateStateInternal();
    }
  },

  onItemChanged:
  function PSB_onItemChanged(aItemId, aProperty, aIsAnnotationProperty,
                             aNewValue, aLastModified, aItemType)
  {
    if (!this._starIcon) {
      return;
    }

    if (aProperty == "uri") {
      let index = this._itemIds.indexOf(aItemId);
      
      
      if (index != -1 && aNewValue != this._uri.spec) {
        this._itemIds.splice(index, 1);
        this._updateStateInternal();
      }
      
      else if (index == -1 && aNewValue == this._uri.spec) {
        this._itemIds.push(aItemId);
        this._updateStateInternal();
      }
    }
  },

  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onBeforeItemRemoved: function () {},
  onItemVisited: function () {},
  onItemMoved: function () {}
};





let PlacesToolbarHelper = {
  _place: "place:folder=TOOLBAR",

  get _viewElt() {
    return document.getElementById("PlacesToolbar");
  },

  init: function PTH_init() {
    let viewElt = this._viewElt;
    if (!viewElt || viewElt._placesView)
      return;

    
    
    let toolbar = viewElt.parentNode.parentNode;
    if (toolbar.collapsed ||
        getComputedStyle(toolbar, "").display == "none")
      return;

    new PlacesToolbar(this._place);
  },

  customizeStart: function PTH_customizeStart() {
    let viewElt = this._viewElt;
    if (viewElt && viewElt._placesView)
      viewElt._placesView.uninit();
  },

  customizeDone: function PTH_customizeDone() {
    this.init();
  }
};



let BookmarksMenuButton = {
  get button() {
    return document.getElementById("bookmarks-menu-button");
  },

  get buttonContainer() {
    return document.getElementById("bookmarks-menu-button-container");
  },

  get personalToolbar() {
    delete this.personalToolbar;
    return this.personalToolbar = document.getElementById("PersonalToolbar");
  },

  get bookmarksToolbarItem() {
    return document.getElementById("personal-bookmarks");
  },

  init: function BMB_init() {
    this.updatePosition();

    
    
  },

  _popupNeedsUpdate: {},
  onPopupShowing: function BMB_onPopupShowing(event) {
    
    if (event.target != event.currentTarget)
      return;

    let popup = event.target;
    let needsUpdate = this._popupNeedsUpdate[popup.id];

    
    
    if (needsUpdate === false)
      return;
    this._popupNeedsUpdate[popup.id] = false;

    function getPlacesAnonymousElement(aAnonId)
      document.getAnonymousElementByAttribute(popup.parentNode,
                                              "placesanonid",
                                              aAnonId);

    let viewToolbarMenuitem = getPlacesAnonymousElement("view-toolbar");
    if (viewToolbarMenuitem) {
      
      viewToolbarMenuitem.setAttribute("checked",
                                       !this.personalToolbar.collapsed);
    }

    let toolbarMenuitem = getPlacesAnonymousElement("toolbar-autohide");
    if (toolbarMenuitem) {
      
      
      toolbarMenuitem.collapsed = toolbarMenuitem.nextSibling.collapsed =
        isElementVisible(this.bookmarksToolbarItem);
    }
  },

  updatePosition: function BMB_updatePosition() {
    
    
    
    for (let popupId in this._popupNeedsUpdate) {
      this._popupNeedsUpdate[popupId] = true;
    }

    let button = this.button;
    if (!button)
      return;

    
    
    let bookmarksToolbarItem = this.bookmarksToolbarItem;
    let bookmarksOnVisibleToolbar = bookmarksToolbarItem &&
                                    !bookmarksToolbarItem.parentNode.collapsed &&
                                    bookmarksToolbarItem.parentNode.getAttribute("autohide") != "true";

    
    
    let container = this.buttonContainer;
    let containerNearBookmarks = container && bookmarksToolbarItem &&
                                 container.parentNode == bookmarksToolbarItem.parentNode;

    if (bookmarksOnVisibleToolbar && !containerNearBookmarks) {
      if (button.parentNode != bookmarksToolbarItem) {
        this._uninitView();
        bookmarksToolbarItem.appendChild(button);
      }
    }
    else {
      if (container && button.parentNode != container) {
        this._uninitView();
        container.appendChild(button);
      }
    }
    this._updateStyle();
  },

  _updateStyle: function BMB__updateStyle() {
    let button = this.button;
    if (!button)
      return;

    let container = this.buttonContainer;
    let containerOnPersonalToolbar = container &&
                                     (container.parentNode == this.personalToolbar ||
                                      container.parentNode.parentNode == this.personalToolbar);

    if (button.parentNode == this.bookmarksToolbarItem ||
        containerOnPersonalToolbar) {
      button.classList.add("bookmark-item");
      button.classList.remove("toolbarbutton-1");
    }
    else {
      button.classList.remove("bookmark-item");
      button.classList.add("toolbarbutton-1");
    }
  },

  _uninitView: function BMB__uninitView() {
    
    
    
    let button = this.button;
    if (button && button._placesView)
      button._placesView.uninit();
  },

  customizeStart: function BMB_customizeStart() {
    this._uninitView();
    let button = this.button;
    let container = this.buttonContainer;
    if (button && container && button.parentNode != container) {
      
      container.appendChild(button);
      this._updateStyle();
    }
  },

  customizeChange: function BMB_customizeChange() {
    this._updateStyle();
  },

  customizeDone: function BMB_customizeDone() {
    this.updatePosition();
  }
};
