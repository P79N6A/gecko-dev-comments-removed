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

  
  QueryInterface: function SU_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsIDOMEventListener) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_NOINTERFACE;
  },

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

  
  _blockedCommands: ["cmd_close", "cmd_closeWindow"],
  _blockCommands: function SU__blockCommands() {
    for each(var key in this._blockedCommands) {
      var elt = this._element(key);
      
      if (elt.hasAttribute("wasDisabled"))
        continue;
      if (elt.getAttribute("disabled") == "true")
        elt.setAttribute("wasDisabled", "true");
      else {
        elt.setAttribute("wasDisabled", "false");
        elt.setAttribute("disabled", "true");
      }
    }
  },

  _restoreCommandsState: function SU__restoreCommandsState() {
    for each(var key in this._blockedCommands) {
      var elt = this._element(key);
      if (elt.getAttribute("wasDisabled") != "true")
        elt.removeAttribute("disabled");
      elt.removeAttribute("wasDisabled");
    }
  },

  
  handleEvent: function SU_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "popuphidden":
        if (aEvent.originalTarget == this.panel) {
          if (!this._element("editBookmarkPanelContent").hidden)
            this.quitEditMode();
          this._restoreCommandsState();
          this._itemId = -1;
          this._uri = null;
          if (this._batching) {
            PlacesUIUtils.ptm.endBatch();
            this._batching = false;
          }
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
    this._element("editBookmarkPanelEditButton").hidden = true;
    this._element("editBookmarkPanelUndoRemoveButton").hidden = true;

    
    
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
    this.panel.openPopup(aAnchorElement, aPosition, -1, -1);

    gEditItemOverlay.initPanel(this._itemId,
                               { hiddenRows: ["description", "location",
                                              "loadInSidebar", "keyword"] });
  },

  panelShown:
  function SU_panelShown(aEvent) {
    if (aEvent.target == this.panel) {
      if (!this._element("editBookmarkPanelContent").hidden) {
        fieldToFocus = "editBMPanel_" +
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

  showPageBookmarkedNotification:
  function PCH_showPageBookmarkedNotification(aItemId, aAnchorElement, aPosition) {
    this._blockCommands(); 

    var brandBundle = this._element("bundle_brand");
    var brandShortName = brandBundle.getString("brandShortName");

    
    this._element("editBookmarkPanelTitle").value =
      gNavigatorBundle.getString("editBookmarkPanel.pageBookmarkedTitle");

    
    this._element("editBookmarkPanelDescription").textContent =
      gNavigatorBundle.getFormattedString("editBookmarkPanel.pageBookmarkedDescription",
                                          [brandShortName]);

    
    
    this._element("editBookmarkPanelEditButton").hidden = false;
    this._element("editBookmarkPanelRemoveButton").hidden = false;
    this._element("editBookmarkPanelUndoRemoveButton").hidden = true;

    
    this._element("editBookmarkPanelStarIcon").removeAttribute("unstarred");

    this._itemId = aItemId !== undefined ? aItemId : this._itemId;
    if (this.panel.state == "closed") {
      
      this.panel.popupBoxObject
          .setConsumeRollupEvent(Ci.nsIPopupBoxObject.ROLLUP_CONSUME);
      this.panel.openPopup(aAnchorElement, aPosition, -1, -1);
    }
    else
      this.panel.focus();
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
    
    
    
    this.panel.hidePopup();
    this.endBatch();
    PlacesUIUtils.ptm.undoTransaction();
  },

  removeBookmarkButtonCommand: function SU_removeBookmarkButtonCommand() {
#ifdef ADVANCED_STARRING_UI
    
    
    
    
    if (this._batching) {
      PlacesUIUtils.ptm.endBatch();
      PlacesUIUtils.ptm.beginBatch(); 

      
      
      this._element("editBookmarkPanelTitle").value =
        gNavigatorBundle.getString("editBookmarkPanel.bookmarkedRemovedTitle");

      
      this.quitEditMode();

      
      
      this._element("editBookmarkPanelUndoRemoveButton").hidden = false;
      this._element("editBookmarkPanelRemoveButton").hidden = true;
      this._element("editBookmarkPanelStarIcon").setAttribute("unstarred", "true");
      this.panel.focus();
    }
#endif

    
    this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);

    
    
    var itemIds = PlacesUtils.getBookmarksForURI(this._uri);
    for (var i=0; i < itemIds.length; i++) {
      var txn = PlacesUIUtils.ptm.removeItem(itemIds[i]);
      PlacesUIUtils.ptm.doTransaction(txn);
    }

#ifdef ADVANCED_STARRING_UI
    
    
    if (!this._batching)
#endif
      this.panel.hidePopup();
  },

  undoRemoveBookmarkCommand: function SU_undoRemoveBookmarkCommand() {
    
    
    this.endBatch();
    PlacesUIUtils.ptm.undoTransaction();
    this._itemId = PlacesUtils.getMostRecentBookmarkForURI(this._uri);
    this.showEditBookmarkPopup();
  },

  beginBatch: function SU_beginBatch() {
    if (!this._batching) {
      PlacesUIUtils.ptm.beginBatch();
      this._batching = true;
    }
  },

  endBatch: function SU_endBatch() {
    if (this._batching) {
      PlacesUIUtils.ptm.endBatch();
      this._batching = false;
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
      var descAnno = { name: DESCRIPTION_ANNO, value: description };
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
        
        
        var position = (getComputedStyle(gNavToolbox, "").direction == "rtl") ? 'after_start' : 'after_end';
        if (aShowEditUI)
          StarUI.showEditBookmarkPopup(itemId, starIcon, position);
#ifdef ADVANCED_STARRING_UI
        else
          StarUI.showPageBookmarkedNotification(itemId, starIcon, position);
#endif
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

  







  _getUniqueTabInfo: function BATC__getUniqueTabInfo() {
    var tabList = [];
    var seenURIs = {};

    var browsers = gBrowser.browsers;
    for (var i = 0; i < browsers.length; ++i) {
      let uri = browsers[i].currentURI;

      
      if (uri.spec in seenURIs)
        continue;

      
      seenURIs[uri.spec] = null;
      tabList.push(uri);
    }
    return tabList;
  },

  



  bookmarkCurrentPages: function PCH_bookmarkCurrentPages() {
    var tabURIs = this._getUniqueTabInfo();
    PlacesUIUtils.showMinimalAddMultiBookmarkUI(tabURIs);
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
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var organizer = wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      
      openDialog("chrome:
                 "", "chrome,toolbar=yes,dialog=no,resizable", aLeftPaneRoot);
    }
    else {
      organizer.PlacesOrganizer.selectLeftPaneQuery(aLeftPaneRoot);
      organizer.focus();
    }
  },

  deleteButtonOnCommand: function PCH_deleteButtonCommand() {
    PlacesUtils.bookmarks.removeItem(gEditItemOverlay.itemId);

    
    PlacesUtils.tagging.untagURI(gEditItemOverlay._uri, null);

    this.panel.hidePopup();
  }
};


var HistoryMenu = {
  get _ss() {
    delete this._ss;
    return this._ss = Cc["@mozilla.org/browser/sessionstore;1"].
                      getService(Ci.nsISessionStore);
  },

  toggleRecentlyClosedTabs: function PHM_toggleRecentlyClosedTabs() {
    
    var undoPopup = document.getElementById("historyUndoPopup");

    
    if (this._ss.getClosedTabCount(window) == 0)
      undoPopup.parentNode.setAttribute("disabled", true);
    else
      undoPopup.parentNode.removeAttribute("disabled");
  },

  





  _undoCloseMiddleClick: function PHM__undoCloseMiddleClick(aEvent) {
    if (aEvent.button != 1)
      return;

    undoCloseTab(aEvent.originalTarget.value);
    gBrowser.moveTabToEnd();
  },

  


  populateUndoSubmenu: function PHM_populateUndoSubmenu() {
    var undoPopup = document.getElementById("historyUndoPopup");

    
    while (undoPopup.hasChildNodes())
      undoPopup.removeChild(undoPopup.firstChild);

    
    if (this._ss.getClosedTabCount(window) == 0) {
      undoPopup.parentNode.setAttribute("disabled", true);
      return;
    }

    
    undoPopup.parentNode.removeAttribute("disabled");

    
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
      m.setAttribute("class", "menuitem-iconic bookmark-item");
      m.setAttribute("value", i);
      m.setAttribute("oncommand", "undoCloseTab(" + i + ");");
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
    m.setAttribute("accesskey", strings.getString("menuRestoreAllTabs.accesskey"));
    m.addEventListener("command", function() {
      for (var i = 0; i < undoItems.length; i++)
        undoCloseTab();
    }, false);
  },

  toggleRecentlyClosedWindows: function PHM_toggleRecentlyClosedWindows() {
    
    let undoPopup = document.getElementById("historyUndoWindowPopup");

    
    if (this._ss.getClosedWindowCount() == 0)
      undoPopup.parentNode.setAttribute("disabled", true);
    else
      undoPopup.parentNode.removeAttribute("disabled");
  },

  


  populateUndoWindowSubmenu: function PHM_populateUndoWindowSubmenu() {
    let undoPopup = document.getElementById("historyUndoWindowPopup");
    let menuLabelString = gNavigatorBundle.getString("menuUndoCloseWindowLabel");
    let menuLabelStringSingleTab =
      gNavigatorBundle.getString("menuUndoCloseWindowSingleTabLabel");

    
    while (undoPopup.hasChildNodes())
      undoPopup.removeChild(undoPopup.firstChild);

    
    if (this._ss.getClosedWindowCount() == 0) {
      undoPopup.parentNode.setAttribute("disabled", true);
      return;
    }

    
    undoPopup.parentNode.removeAttribute("disabled");

    
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
      m.setAttribute("class", "menuitem-iconic bookmark-item");
      m.setAttribute("oncommand", "undoCloseWindow(" + i + ");");
      if (i == 0)
        m.setAttribute("key", "key_undoCloseWindow");
      undoPopup.appendChild(m);
    }

    
    undoPopup.appendChild(document.createElement("menuseparator"));
    let m = undoPopup.appendChild(document.createElement("menuitem"));
    m.id = "menu_restoreAllWindows";
    m.setAttribute("label", gNavigatorBundle.getString("menuRestoreAllWindows.label"));
    m.setAttribute("accesskey", gNavigatorBundle.getString("menuRestoreAllWindows.accesskey"));
    m.setAttribute("oncommand",
      "for (var i = 0; i < " + undoItems.length + "; i++) undoCloseWindow();");
  },

  




  onPopupShowing: function PHM_onPopupShowing(aEvent) {
    
    if (aEvent.target != aEvent.currentTarget)
      return;

    var menuPopup = aEvent.target;
    var resultNode = menuPopup.getResultNode();
    resultNode.containerOpen = true;
    document.getElementById("endHistorySeparator").hidden =
      resultNode.childCount == 0;

    this.toggleRecentlyClosedTabs();
    this.toggleRecentlyClosedWindows();
  },

  




  onPopupHidden: function PHM_onPopupHidden(aEvent) {
    
    if (aEvent.target != aEvent.currentTarget)
      return;

    var menuPopup = aEvent.target;
    var resultNode = menuPopup.getResultNode();
    if (resultNode.containerOpen)
      resultNode.containerOpen = false;
  }
};




var BookmarksEventHandler = {  
  








  onClick: function BT_onClick(aEvent) {
    
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
        else if (node.localName != "menu")
          break;
      }
    }

    if (target.node && PlacesUtils.nodeIsContainer(target.node)) {
      
      
      
      if (target.localName == "menu" || target.localName == "toolbarbutton")
        PlacesUIUtils.openContainerNodeInTabs(target.node, aEvent);
    }
    else if (aEvent.button == 1) {
      
      this.onCommand(aEvent);
    }
  },

  






  onCommand: function BM_onCommand(aEvent) {
    var target = aEvent.originalTarget;
    if (target.node)
      PlacesUIUtils.openNodeWithEvent(target.node, aEvent);
  },

  






  onPopupShowing: function BM_onPopupShowing(event) {
    var target = event.originalTarget;
    if (!target.hasAttribute("placespopup"))
      return;

    
    var numNodes = 0;
    var hasMultipleURIs = false;
    var currentChild = target.firstChild;
    while (currentChild) {
      if (currentChild.localName == "menuitem" && currentChild.node) {
        if (++numNodes == 2) {
          hasMultipleURIs = true;
          break;
        }
      }
      currentChild = currentChild.nextSibling;
    }

    var itemId = target._resultNode.itemId;
    var siteURIString = "";
    if (itemId != -1 && PlacesUtils.itemIsLivemark(itemId)) {
      var siteURI = PlacesUtils.livemarks.getSiteURI(itemId);
      if (siteURI)
        siteURIString = siteURI.spec;
    }

    if (!siteURIString && target._endOptOpenSiteURI) {
        target.removeChild(target._endOptOpenSiteURI);
        target._endOptOpenSiteURI = null;
    }

    if (!hasMultipleURIs && target._endOptOpenAllInTabs) {
      target.removeChild(target._endOptOpenAllInTabs);
      target._endOptOpenAllInTabs = null;
    }

    if (!(hasMultipleURIs || siteURIString)) {
      
      if (target._endOptSeparator) {
        target.removeChild(target._endOptSeparator);
        target._endOptSeparator = null;
        target._endMarker = -1;
      }
      return;
    }

    if (!target._endOptSeparator) {
      
      target._endOptSeparator = document.createElement("menuseparator");
      target._endOptSeparator.className = "bookmarks-actions-menuseparator";
      target._endMarker = target.childNodes.length;
      target.appendChild(target._endOptSeparator);
    }

    if (siteURIString && !target._endOptOpenSiteURI) {
      
      target._endOptOpenSiteURI = document.createElement("menuitem");
      target._endOptOpenSiteURI.className = "openlivemarksite-menuitem";
      target._endOptOpenSiteURI.setAttribute("targetURI", siteURIString);
      target._endOptOpenSiteURI.setAttribute("oncommand",
          "openUILink(this.getAttribute('targetURI'), event);");
      
      
      
      
      target._endOptOpenSiteURI.setAttribute("onclick",
          "checkForMiddleClick(this, event); event.stopPropagation();");
      target._endOptOpenSiteURI.setAttribute("label",
          PlacesUIUtils.getFormattedString("menuOpenLivemarkOrigin.label",
          [target.parentNode.getAttribute("label")]));
      target.appendChild(target._endOptOpenSiteURI);
    }

    if (hasMultipleURIs && !target._endOptOpenAllInTabs) {
        
        
        target._endOptOpenAllInTabs = document.createElement("menuitem");
        target._endOptOpenAllInTabs.className = "openintabs-menuitem";
        target._endOptOpenAllInTabs.setAttribute("oncommand",
            "PlacesUIUtils.openContainerNodeInTabs(this.parentNode._resultNode, event);");
        target._endOptOpenAllInTabs.setAttribute("onclick",
            "checkForMiddleClick(this, event); event.stopPropagation();");
        target._endOptOpenAllInTabs.setAttribute("label",
            gNavigatorBundle.getString("menuOpenAllInTabs.label"));
        target.appendChild(target._endOptOpenAllInTabs);
    }
  },

  fillInBHTooltip: function(aDocument, aEvent) {
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
      if (tooltipNode.node)
        node = tooltipNode.node;
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





var BookmarksMenuDropHandler = {
  




  onDragOver: function BMDH_onDragOver(event, flavor, session) {
    if (!this.canDrop(event, session))
      event.dataTransfer.effectAllowed = "none";
  },

  




  getSupportedFlavours: function BMDH_getSupportedFlavours() {
    var view = document.getElementById("bookmarksMenuPopup");
    return view.getSupportedFlavours();
  },

  








  canDrop: function BMDH_canDrop(event, session) {
    PlacesControllerDragHelper.currentDataTransfer = event.dataTransfer;

    var ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId, -1);  
    return ip && PlacesControllerDragHelper.canDrop(ip);
  },

  








  onDrop: function BMDH_onDrop(event, data, session) {
    PlacesControllerDragHelper.currentDataTransfer = event.dataTransfer;

  
    var ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId, -1,
                                Ci.nsITreeView.DROP_ON);
    PlacesControllerDragHelper.onDrop(ip);
  },

  




  onDragExit: function BMDH_onDragExit(event, session) {
    PlacesControllerDragHelper.currentDataTransfer = null;
  }
};





var PlacesMenuDNDController = {
  _springLoadDelay: 350, 

  


  _timers: { },
  
  




  onBookmarksMenuDragEnter: function PMDC_onDragEnter(event) {
    if ("loadTime" in this._timers) 
      return;
    
    this._setDragTimer("loadTime", this._openBookmarksMenu, 
                       this._springLoadDelay, [event]);
  },
  
  










  _setDragTimer: function PMDC__setDragTimer(id, callback, delay, args) {
    if (!this._dragSupported)
      return;

    
    if (id in this._timers)
      this._timers[id].cancel();
      
    



    function Callback(object, method, args) {
      this._method = method;
      this._args = args;
      this._object = object;
    }
    Callback.prototype = {
      notify: function C_notify(timer) {
        this._method.apply(this._object, this._args);
      }
    };
    
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(new Callback(this, callback, args), delay, 
                           timer.TYPE_ONE_SHOT);
    this._timers[id] = timer;
  },
  
  




  _isContainer: function PMDC__isContainer(node) {
    return node.localName == "menu" ||
           (node.localName == "toolbarbutton" &&
            node.getAttribute("type") == "menu");
  },
  
  






  _openBookmarksMenu: function PMDC__openBookmarksMenu(event) {
    if ("loadTime" in this._timers)
      delete this._timers.loadTime;
    if (event.target.id == "bookmarksMenu") {
      
      event.target.lastChild.setAttribute("autoopened", "true");
      event.target.lastChild.showPopup(event.target.lastChild);
    }  
  },

  
  
#ifdef XP_MACOSX
  _dragSupported: false
#else
  _dragSupported: true
#endif
};

var PlacesStarButton = {
  init: function PSB_init() {
    try {
      PlacesUtils.bookmarks.addObserver(this, false);
    } catch(ex) {
      Components.utils.reportError("PlacesStarButton.init(): error adding bookmark observer: " + ex);
    }
  },

  uninit: function PSB_uninit() {
    PlacesUtils.bookmarks.removeObserver(this);
  },

  QueryInterface: function PSB_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Cr.NS_NOINTERFACE;
  },

  _starred: false,
  _batching: false,

  updateState: function PSB_updateState() {
    var starIcon = document.getElementById("star-button");
    if (!starIcon)
      return;

    var uri = gBrowser.currentURI;
    this._starred = uri && (PlacesUtils.getMostRecentBookmarkForURI(uri) != -1 ||
                            PlacesUtils.getMostRecentFolderForFeedURI(uri) != -1);
    if (this._starred) {
      starIcon.setAttribute("starred", "true");
      starIcon.setAttribute("tooltiptext", gNavigatorBundle.getString("starButtonOn.tooltip"));
    }
    else {
      starIcon.removeAttribute("starred");
      starIcon.setAttribute("tooltiptext", gNavigatorBundle.getString("starButtonOff.tooltip"));
    }
  },

  onClick: function PSB_onClick(aEvent) {
    if (aEvent.button == 0)
      PlacesCommandHook.bookmarkCurrentPage(this._starred);

    
    aEvent.stopPropagation();
  },

  
  onBeginUpdateBatch: function PSB_onBeginUpdateBatch() {
    this._batching = true;
  },

  onEndUpdateBatch: function PSB_onEndUpdateBatch() {
    this.updateState();
    this._batching = false;
  },
  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolder, aIndex, aItemType) {
    if (!this._batching && !this._starred)
      this.updateState();
  },

  onBeforeItemRemoved: function() {},

  onItemRemoved: function PSB_onItemRemoved(aItemId, aFolder, aIndex,
                                            aItemType) {
    if (!this._batching)
      this.updateState();
  },

  onItemChanged: function PSB_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aNewValue,
                                            aLastModified, aItemType) {
    if (!this._batching && aProperty == "uri")
      this.updateState();
  },

  onItemVisited: function() { },
  onItemMoved: function() { }
};
