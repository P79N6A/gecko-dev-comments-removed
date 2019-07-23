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
    element.addEventListener("keypress", this, true);
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
            gEditItemOverlay.uninitPanel(true);
          this._restoreCommandsState();
          this._itemId = -1;
          this._uri = null;
          if (this._batching) {
            PlacesUtils.ptm.endBatch();
            this._batching = false;
          }
        }
        break;
      case "keypress":
        if (aEvent.keyCode == KeyEvent.DOM_VK_ESCAPE) {
          
          if (!this._element("editBookmarkPanelContent").hidden)
            this.cancelButtonOnCommand();
          else 
            this.panel.hidePopup();
        }
        else if (aEvent.keyCode == KeyEvent.DOM_VK_RETURN)
          this.panel.hidePopup(); 
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
    this._blockCommands(); 

    var bundle = this._element("bundle_browser");

    
    this._element("editBookmarkPanelTitle").value =
      bundle.getString("editBookmarkPanel.pageBookmarkedTitle");

    
    
    this._element("editBookmarkPanelDescription").textContent = "";
    this._element("editBookmarkPanelBottomButtons").hidden = false;
    this._element("editBookmarkPanelContent").hidden = false;
    this._element("editBookmarkPanelEditButton").hidden = true;
    this._element("editBookmarkPanelUndoRemoveButton").hidden = true;

    
    
    this._element("editBookmarkPanelRemoveButton").hidden = this._batching;

    
    this._element("editBookmarkPanelStarIcon").removeAttribute("unstarred");

    this._itemId = aItemId !== undefined ? aItemId : this._itemId;
    this.beginBatch();

    
    
    
    
    
    PlacesUtils.ptm.doTransaction({ doTransaction: function() { },
                                    undoTransaction: function() { },
                                    redoTransaction: function() { },
                                    isTransient: false,
                                    merge: function() { return false; } });

    if (this.panel.state == "closed") {
      
      this.panel.popupBoxObject
          .setConsumeRollupEvent(Ci.nsIPopupBoxObject.ROLLUP_CONSUME);
      this.panel.openPopup(aAnchorElement, aPosition, -1, -1);
    }
    else {
      var namePicker = this._element("editBMPanel_namePicker");
      namePicker.focus();
      namePicker.editor.selectAll();
    }

    gEditItemOverlay.initPanel(this._itemId,
                               { hiddenRows: ["description", "location",
                                              "loadInSidebar", "keyword"] });
  },

  panelShown:
  function SU_panelShown(aEvent) {
    if (aEvent.target == this.panel) {
      if (!this._element("editBookmarkPanelContent").hidden) {
        var namePicker = this._element("editBMPanel_namePicker");
        namePicker.focus();
        namePicker.editor.selectAll();
      }
      else
        this.panel.focus();
    }
  },

  showPageBookmarkedNotification:
  function PCH_showPageBookmarkedNotification(aItemId, aAnchorElement, aPosition) {
    this._blockCommands(); 

    var bundle = this._element("bundle_browser");
    var brandBundle = this._element("bundle_brand");
    var brandShortName = brandBundle.getString("brandShortName");

    
    this._element("editBookmarkPanelTitle").value =
      bundle.getString("editBookmarkPanel.pageBookmarkedTitle");

    
    this._element("editBookmarkPanelDescription").textContent =
      bundle.getFormattedString("editBookmarkPanel.pageBookmarkedDescription",
                                [brandShortName]);

    
    this._element("editBookmarkPanelContent").hidden = true;
    this._element("editBookmarkPanelBottomButtons").hidden = true;

    
    
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

  editButtonCommand: function SU_editButtonCommand() {
    this.showEditBookmarkPopup();
  },

  cancelButtonOnCommand: function SU_cancelButtonOnCommand() {
    this.endBatch();
    PlacesUtils.ptm.undoTransaction();
    this.panel.hidePopup();
  },

  removeBookmarkButtonCommand: function SU_removeBookmarkButtonCommand() {
#ifdef ADVANCED_STARRING_UI
    
    
    
    
    if (this._batching) {
      PlacesUtils.ptm.endBatch();
      PlacesUtils.ptm.beginBatch(); 
      var bundle = this._element("bundle_browser");

      
      
      this._element("editBookmarkPanelTitle").value =
        bundle.getString("editBookmarkPanel.bookmarkedRemovedTitle");
      
      
      this._element("editBookmarkPanelContent").hidden = true;
      this._element("editBookmarkPanelBottomButtons").hidden = true;
      this._element("editBookmarkPanelUndoRemoveButton").hidden = false;
      this._element("editBookmarkPanelRemoveButton").hidden = true;
      this._element("editBookmarkPanelStarIcon").setAttribute("unstarred", "true");
      this.panel.focus();
    }
#endif

    
    this._uri = PlacesUtils.bookmarks.getBookmarkURI(this._itemId);

    
    
    var itemIds = PlacesUtils.getBookmarksForURI(this._uri);
    for (var i=0; i < itemIds.length; i++) {
      var txn = PlacesUtils.ptm.removeItem(itemIds[i]);
      PlacesUtils.ptm.doTransaction(txn);
    }

#ifdef ADVANCED_STARRING_UI
    
    
    if (!this._batching)
#endif
      this.panel.hidePopup();
  },

  undoRemoveBookmarkCommand: function SU_undoRemoveBookmarkCommand() {
    
    
    this.endBatch();
    PlacesUtils.ptm.undoTransaction();
    this._itemId = PlacesUtils.getMostRecentBookmarkForURI(this._uri);
    this.showEditBookmarkPopup();
  },

  beginBatch: function SU_beginBatch() {
    if (!this._batching) {
      PlacesUtils.ptm.beginBatch();
      this._batching = true;
    }
  },

  endBatch: function SU_endBatch() {
    if (this._batching) {
      PlacesUtils.ptm.endBatch();
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
      try {
        title = webNav.document.title || url.spec;
        description = PlacesUtils.getDescriptionFromDocument(webNav.document);
      }
      catch (e) { }

      if (aShowEditUI) {
        
        
        
        StarUI.beginBatch();
      }

      var parent = aParent != undefined ?
                   aParent : PlacesUtils.unfiledBookmarksFolderId;
      var descAnno = { name: DESCRIPTION_ANNO, value: description };
      var txn = PlacesUtils.ptm.createItem(uri, parent, -1,
                                           title, null, [descAnno]);
      PlacesUtils.ptm.doTransaction(txn);
      itemId = PlacesUtils.getMostRecentBookmarkForURI(uri);
    }

    
    
    if (aBrowser.contentWindow == window.content) {
      var starIcon = aBrowser.ownerDocument.getElementById("star-button");
      if (starIcon && isElementVisible(starIcon)) {
        if (aShowEditUI)
          StarUI.showEditBookmarkPopup(itemId, starIcon, "after_end");
#ifdef ADVANCED_STARRING_UI
        else
          StarUI.showPageBookmarkedNotification(itemId, starIcon, "after_end");
#endif
        return;
      }
    }

    StarUI.showEditBookmarkPopup(itemId, aBrowser, "overlap");
  },

  


  bookmarkCurrentPage: function PCH_bookmarkCurrentPage(aShowEditUI, aParent) {
    this.bookmarkPage(getBrowser().selectedBrowser, aParent, aShowEditUI);
  },

  









  bookmarkLink: function PCH_bookmarkLink(aParent, aURL, aTitle) {
    var linkURI = makeURI(aURL);
    var itemId = PlacesUtils.getMostRecentBookmarkForURI(linkURI);
    if (itemId == -1) {
      StarUI.beginBatch();
      var txn = PlacesUtils.ptm.createItem(linkURI, aParent, -1, aTitle);
      PlacesUtils.ptm.doTransaction(txn);
      itemId = PlacesUtils.getMostRecentBookmarkForURI(linkURI);
    }

    StarUI.showEditBookmarkPopup(itemId, getBrowser(), "overlap");
  },

  







  _getUniqueTabInfo: function BATC__getUniqueTabInfo() {
    var tabList = [];
    var seenURIs = [];

    var browsers = getBrowser().browsers;
    for (var i = 0; i < browsers.length; ++i) {
      var webNav = browsers[i].webNavigation;
      var uri = webNav.currentURI;

      
      if (uri.spec in seenURIs)
        continue;

      
      seenURIs[uri.spec] = true;
      tabList.push(uri);
    }
    return tabList;
  },

  



  bookmarkCurrentPages: function PCH_bookmarkCurrentPages() {
    var tabURIs = this._getUniqueTabInfo();
    PlacesUtils.showMinimalAddMultiBookmarkUI(tabURIs);
  },

  
  








  addLiveBookmark: function PCH_addLiveBookmark(url, feedTitle, feedSubtitle) {
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var feedURI = ios.newURI(url, null, null);
    
    var doc = gBrowser.contentDocument;
    var title = (arguments.length > 1) ? feedTitle : doc.title;
 
    var description;
    if (arguments.length > 2)
      description = feedSubtitle;
    else
      description = PlacesUtils.getDescriptionFromDocument(doc);

    var toolbarIP =
      new InsertionPoint(PlacesUtils.bookmarks.toolbarFolder, -1);
    PlacesUtils.showMinimalAddLivemarkUI(feedURI, gBrowser.currentURI,
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
  




  onPopupShowing: function PHM_onPopupShowing(aMenuPopup) {
    var resultNode = aMenuPopup.getResultNode();
    var wasOpen = resultNode.containerOpen;
    resultNode.containerOpen = true;
    document.getElementById("endHistorySeparator").hidden =
      resultNode.childCount == 0;

    if (!wasOpen)
      resultNode.containerOpen = false;

    
    this.toggleRecentlyClosedTabs();
  }
};




var BookmarksEventHandler = {  
  








  onClick: function BT_onClick(aEvent) {
    
    if (aEvent.button != 1)
      return;

    var target = aEvent.originalTarget;
    var view = PlacesUtils.getViewForNode(target);
    if (target.node && PlacesUtils.nodeIsFolder(target.node)) {
      
      
      
      if (target.localName == "menu" || target.localName == "toolbarbutton")
        PlacesUtils.openContainerNodeInTabs(target.node, aEvent);
    }
    else
      this.onCommand(aEvent);

    
    
    if (target.localName == "menu" ||
        target.localName == "menuitem") {
      var node = target.parentNode;
      while (node && 
             (node.localName == "menu" || 
              node.localName == "menupopup")) {
        if (node.localName == "menupopup")
          node.hidePopup();

        node = node.parentNode;
      }
    }
    
    
    
    
    var bookmarksBar = document.getElementById("bookmarksBarContent");
    if (bookmarksBar._chevron.getAttribute("open") == "true")
      bookmarksBar._chevron.firstChild.hidePopup();
  },

  






  onCommand: function BM_onCommand(aEvent) {
    var target = aEvent.originalTarget;
    if (target.node)
      PlacesUtils.openNodeWithEvent(target.node, aEvent);
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
    if (itemId != -1 && PlacesUtils.livemarks.isLivemark(itemId)) {
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
      target._endOptSeparator.setAttribute("builder", "end");
      target._endMarker = target.childNodes.length;
      target.appendChild(target._endOptSeparator);
    }

    if (siteURIString && !target._endOptOpenSiteURI) {
      
      target._endOptOpenSiteURI = document.createElement("menuitem");
      target._endOptOpenSiteURI.setAttribute("siteURI", siteURIString);
      target._endOptOpenSiteURI.setAttribute("oncommand",
          "openUILink(this.getAttribute('siteURI'), event);");
      
      
      
      
      target._endOptOpenSiteURI.setAttribute("onclick",
          "checkForMiddleClick(this, event); event.stopPropagation();");
      target._endOptOpenSiteURI.setAttribute("label",
          PlacesUtils.getFormattedString("menuOpenLivemarkOrigin.label",
          [target.parentNode.getAttribute("label")]));
      target.appendChild(target._endOptOpenSiteURI);
    }

    if (hasMultipleURIs && !target._endOptOpenAllInTabs) {
        
        
        target._endOptOpenAllInTabs = document.createElement("menuitem");
        target._endOptOpenAllInTabs.setAttribute("oncommand",
            "PlacesUtils.openContainerNodeInTabs(this.parentNode._resultNode, event);");
        target._endOptOpenAllInTabs.setAttribute("label",
            gNavigatorBundle.getString("menuOpenAllInTabs.label"));
        target.appendChild(target._endOptOpenAllInTabs);
    }
  },

  fillInBTTooltip: function(aTipElement) {
    
    if (aTipElement.localName != "toolbarbutton")
      return false;

    
    if (!PlacesUtils.nodeIsURI(aTipElement.node))
      return false;

    var url = aTipElement.node.uri;
    if (!url) 
      return false;

    var tooltipUrl = document.getElementById("btUrlText");
    tooltipUrl.value = url;

    var title = aTipElement.label;
    var tooltipTitle = document.getElementById("btTitleText");
    if (title && title != url) {
      tooltipTitle.hidden = false;
      tooltipTitle.value = title;
    }
    else
      tooltipTitle.hidden = true;

    
    return true;
  }
};





var BookmarksMenuDropHandler = {
  




  onDragOver: function BMDH_onDragOver(event, flavor, session) {
    session.canDrop = this.canDrop(event, session);
  },

  




  getSupportedFlavours: function BMDH_getSupportedFlavours() {
    var view = document.getElementById("bookmarksMenuPopup");
    return view.getSupportedFlavours();
  },

  








  canDrop: function BMDH_canDrop(event, session) {
    return PlacesControllerDragHelper.canDrop();
  },

  








  onDrop: function BMDH_onDrop(event, data, session) {
    
    var ip = new InsertionPoint(PlacesUtils.bookmarksMenuFolderId, -1);
    PlacesControllerDragHelper.onDrop(ip);
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
           node.localName == "toolbarbutton" && node.getAttribute("type") == "menu";
  },
  
  






  _openBookmarksMenu: function PMDC__openBookmarksMenu(event) {
    if ("loadTime" in this._timers)
      delete this._timers.loadTime;
    if (event.target.id == "bookmarksMenu") {
      
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
    PlacesUtils.bookmarks.addObserver(this, false);
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

    var browserBundle = document.getElementById("bundle_browser");
    var uri = getBrowser().currentURI;
    this._starred = uri && (PlacesUtils.getMostRecentBookmarkForURI(uri) != -1 ||
                            PlacesUtils.getMostRecentFolderForFeedURI(uri) != -1);
    if (this._starred) {
      starIcon.setAttribute("starred", "true");
      starIcon.setAttribute("tooltiptext", browserBundle.getString("starButtonOn.tooltip"));
    }
    else {
      starIcon.removeAttribute("starred");
      starIcon.setAttribute("tooltiptext", browserBundle.getString("starButtonOff.tooltip"));
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
  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolder, aIndex) {
    if (!this._batching && !this._starred)
      this.updateState();
  },

  onItemRemoved: function PSB_onItemRemoved(aItemId, aFolder, aIndex) {
    if (!this._batching)
      this.updateState();
  },

  onItemChanged: function PSB_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aValue) {
    if (!this._batching && aProperty == "uri")
      this.updateState();
  },

  onItemVisited: function() { },
  onItemMoved: function() { }
};




function placesMigrationTasks() {
  
  
  if (gPrefService.getBoolPref("browser.places.migratePostDataAnnotations")) {
    const annosvc = PlacesUtils.annotations;
    var bmsvc = PlacesUtils.bookmarks;
    const oldPostDataAnno = "URIProperties/POSTData";
    var pages = annosvc.getPagesWithAnnotation(oldPostDataAnno, {});
    for (let i = 0; i < pages.length; i++) {
      try {
        let uri = pages[i];
        var postData = annosvc.getPageAnnotation(uri, oldPostDataAnno);
        
        
        
        
        
        let bookmarks = bmsvc.getBookmarkIdsForURI(uri, {});
        for (let i = 0; i < bookmarks.length; i++) {
          var keyword = bmsvc.getKeywordForBookmark(bookmarks[i]);
          if (keyword)
            annosvc.setItemAnnotation(bookmarks[i], POST_DATA_ANNO, postData, 0, annosvc.EXPIRE_NEVER); 
        }
        
        annosvc.removePageAnnotation(uri, oldPostDataAnno);
      } catch(ex) {}
    }
    gPrefService.setBoolPref("browser.places.migratePostDataAnnotations", false);
  }

  if (gPrefService.getBoolPref("browser.places.updateRecentTagsUri")) {
    var bmsvc = PlacesUtils.bookmarks;
    var tagsFolder = bmsvc.tagsFolder;
    var oldUriSpec = "place:folder=" + tagsFolder + "&group=3&queryType=1"+
                     "&applyOptionsToContainers=1&sort=12&maxResults=10";

    var maxResults = 10;
    var newUriSpec = "place:type=" + 
                     Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                     "&sort=" + 
                     Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING +
                     "&maxResults=" + maxResults;
                     
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

    var oldUri = ios.newURI(oldUriSpec, null, null);
    var newUri = ios.newURI(newUriSpec, null, null);

    let bookmarks = bmsvc.getBookmarkIdsForURI( oldUri, {});
    for (let i = 0; i < bookmarks.length; i++) {
      bmsvc.changeBookmarkURI( bookmarks[i], newUri);
    }
    gPrefService.setBoolPref("browser.places.updateRecentTagsUri", false);
  }
}
