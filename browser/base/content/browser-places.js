






































var PlacesCommandHook = {
  






  bookmarkLink: function PCH_bookmarkLink(url, title) {
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var linkURI = ios.newURI(url, null, null);

    PlacesUtils.showMinimalAddBookmarkUI(linkURI, title);
  },

  




  bookmarkPage: function PCH_bookmarkPage(aBrowser) {
    
    
    
    
    
    
    var webNav = aBrowser.webNavigation;
    var url = webNav.currentURI;
    var title;
    var description;
    try {
      title = webNav.document.title;
      description = PlacesUtils.getDescriptionFromDocument(webNav.document);
    }
    catch (e) { }
    PlacesUtils.showMinimalAddBookmarkUI(url, title, description);
  },

  


  bookmarkCurrentPage: function PCH_bookmarkCurrentPage() {
    this.bookmarkPage(getBrowser().selectedBrowser);
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

  







  showPlacesOrganizer: function PCH_showPlacesOrganizer(aPlace, aForcePlace) {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var organizer = wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      
      openDialog("chrome://browser/content/places/places.xul", 
                 "", "chrome,toolbar=yes,dialog=no,resizable", aPlace);
    }
    else {
      if (aForcePlace)
        organizer.selectPlaceURI(aPlace);

      organizer.focus();
    }
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
    if (PlacesUtils.nodeIsFolder(view.selectedNode)) {
      
      
      
      if (!view.controller.rootNodeIsSelected())
        view.controller.openLinksInTabs();
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
      bookmarksBar._chevron.firstChild.hidePopupAndChildPopups();
  },
  
  






  onCommand: function BM_onCommand(aEvent) {
    
    

    var target = aEvent.originalTarget;
    if (target.hasAttribute("openInTabs"))
      PlacesUtils.getViewForNode(target).controller.openLinksInTabs();
    else if (target.hasAttribute("siteURI"))
      openUILink(target.getAttribute("siteURI"), aEvent);
    
    else
      PlacesUtils.getViewForNode(target)
                 .controller
                 .openSelectedNodeWithEvent(aEvent);
  },

  






  onPopupShowing: function BM_onPopupShowing(event) {
    var target = event.originalTarget;
    if (target.localName == "menupopup" &&
        target.id != "bookmarksMenuPopup" &&
        target.getAttribute("anonid") != "chevronPopup") {
      
      
      
      var numNodes = 0;
      var hasMultipleEntries = false;
      var hasFeedHomePage = false;
      var currentChild = target.firstChild;
      while (currentChild) {
        if (currentChild.localName == "menuitem" && currentChild.node)
          numNodes++;

        
        if (currentChild.getAttribute("openInTabs") == "true")
          return;
        if (currentChild.hasAttribute("siteURI"))
          return;

        currentChild = currentChild.nextSibling;
      }
      if (numNodes > 1)
        hasMultipleEntries = true;

      var button = target.parentNode;
      if (button.getAttribute("livemark") == "true" &&
          button.hasAttribute("siteURI"))
        hasFeedHomePage = true;

      if (hasMultipleEntries || hasFeedHomePage) {
        var separator = document.createElement("menuseparator");
        target.appendChild(separator);

        if (hasFeedHomePage) {
          var openHomePage = document.createElement("menuitem");
          openHomePage.setAttribute(
            "siteURI", button.getAttribute("siteURI"));
          openHomePage.setAttribute(
            "label",
            PlacesUtils.getFormattedString("menuOpenLivemarkOrigin.label",
                                           [button.getAttribute("label")]));
          target.appendChild(openHomePage);
        }

        if (hasMultipleEntries) {
          var openInTabs = document.createElement("menuitem");
          openInTabs.setAttribute("openInTabs", "true");
          openInTabs.setAttribute("label",
                     gNavigatorBundle.getString("menuOpenAllInTabs.label"));
          target.appendChild(openInTabs);
        }
      }
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
    var flavorSet = new FlavourSet();
    var view = document.getElementById("bookmarksMenuPopup");
    for (var i = 0; i < view.peerDropTypes.length; ++i)
      flavorSet.appendFlavour(view.peerDropTypes[i]);
    return flavorSet;
  }, 

  








  canDrop: function BMDH_canDrop(event, session) {
    var view = document.getElementById("bookmarksMenuPopup");
    return PlacesControllerDragHelper.canDrop(view, -1);
  },
  
  








  onDrop: function BMDH_onDrop(event, data, session) {
    var view = document.getElementById("bookmarksMenuPopup");

    
    
    NS_ASSERT(view.insertionPoint.index == -1, "Insertion point for an menupopup view during a drag must be -1!");
    PlacesControllerDragHelper.onDrop(null, view, view.insertionPoint, 1);
    view._rebuild();
  }
};





var PlacesMenuDNDController = {
  
  




  init: function PMDC_init() {
    var placesContext = document.getElementById("placesContext");
    var self = this;
    placesContext.addEventListener("popuphidden", function () { self._closePopups() }, false);
  },

  _springLoadDelay: 350, 

  


  _timers: { },
  
  




  onBookmarksMenuDragEnter: function PMDC_onDragEnter(event) {
    if ("loadTime" in this._timers) 
      return;
    
    this._setDragTimer("loadTime", this._openBookmarksMenu, 
                       this._springLoadDelay, [event]);
  },
  
  





  onDragExit: function PMDC_onDragExit(event) {
    
    if ("closeTime" in this._timers)
      return;
      
    this._setDragTimer("closeTime", this._closePopups, 
                       this._springLoadDelay, [event.target]);
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
  
  





  _closePopups: function PMDC__closePopups(target) {
    if (PlacesControllerDragHelper.draggingOverChildNode(target))
      return;

    if ("closeTime" in this._timers)
      delete this._timers.closeTime;
    
    
    var bookmarksMenu = document.getElementById("bookmarksMenu");
    bookmarksMenu.firstChild.hidePopupAndChildPopups();

    var bookmarksBar = document.getElementById("bookmarksBarContent");
    if (bookmarksBar) {
      
      bookmarksBar._chevron.firstChild.hidePopupAndChildPopups();

      
      var toolbarItems = bookmarksBar.childNodes;
      for (var i = 0; i < toolbarItems.length; ++i) {
        var item = toolbarItems[i]
        if (this._isContainer(item))
          item.firstChild.hidePopupAndChildPopups();
      }
    }
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
