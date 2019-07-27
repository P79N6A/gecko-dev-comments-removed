





XPCOMUtils.defineLazyModuleGetter(this, "ReadingList",
  "resource:///modules/readinglist/ReadingList.jsm");

const READINGLIST_COMMAND_ID = "readingListSidebar";

let ReadingListUI = {
  



  MESSAGES: [
    "ReadingList:GetVisibility",
    "ReadingList:ToggleVisibility",
    "ReadingList:ShowIntro",
  ],

  



  toolbarButton: null,

  




  listenerRegistered: false,

  


  init() {
    this.toolbarButton = document.getElementById("readinglist-addremove-button");

    Preferences.observe("browser.readinglist.enabled", this.updateUI, this);

    const mm = window.messageManager;
    for (let msg of this.MESSAGES) {
      mm.addMessageListener(msg, this);
    }

    this.updateUI();
  },

  


  uninit() {
    Preferences.ignore("browser.readinglist.enabled", this.updateUI, this);

    const mm = window.messageManager;
    for (let msg of this.MESSAGES) {
      mm.removeMessageListener(msg, this);
    }

    if (this.listenerRegistered) {
      ReadingList.removeListener(this);
      this.listenerRegistered = false;
    }
  },

  



  get enabled() {
    return Preferences.get("browser.readinglist.enabled", false);
  },

  



  get isSidebarOpen() {
    return SidebarUI.isOpen && SidebarUI.currentID == READINGLIST_COMMAND_ID;
  },

  



  updateUI() {
    let enabled = this.enabled;
    if (enabled) {
      
      ReadingList.addListener(this);
      this.listenerRegistered = true;
    } else {
      if (this.listenerRegistered) {
        
        
        ReadingList.removeListener(this);
        this.listenerRegistered = false;
      }

      this.hideSidebar();
    }

    document.getElementById(READINGLIST_COMMAND_ID).setAttribute("hidden", !enabled);
    document.getElementById(READINGLIST_COMMAND_ID).setAttribute("disabled", !enabled);
  },

  



  showSidebar() {
    if (this.enabled) {
      return SidebarUI.show(READINGLIST_COMMAND_ID);
    }
  },

  


  hideSidebar() {
    if (this.isSidebarOpen) {
      SidebarUI.hide();
    }
  },

  




  onReadingListPopupShowing: Task.async(function* (target) {
    if (target.id == "BMB_readingListPopup") {
      
      
      document.getElementById("BMB_viewReadingListSidebar")
              .classList.add("panel-subview-footer");
    }

    while (!target.firstChild.id)
      target.firstChild.remove();

    let classList = "menuitem-iconic bookmark-item menuitem-with-favicon";
    let insertPoint = target.firstChild;
    if (insertPoint.classList.contains("subviewbutton"))
      classList += " subviewbutton";

    let hasItems = false;
    yield ReadingList.forEachItem(item => {
      hasItems = true;

      let menuitem = document.createElement("menuitem");
      menuitem.setAttribute("label", item.title || item.url);
      menuitem.setAttribute("class", classList);

      let node = menuitem._placesNode = {
        
        
        type: Ci.nsINavHistoryResultNode.RESULT_TYPE_URI,

        
        
        parent: {type: Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER},

        
        
        
        
        itemId: -1,

        
        uri: item.url,

        
        title: item.title
      };

      Favicons.getFaviconURLForPage(item.uri, uri => {
        if (uri) {
          menuitem.setAttribute("image",
                                Favicons.getFaviconLinkForIcon(uri).spec);
        }
      });

      target.insertBefore(menuitem, insertPoint);
    }, {sort: "addedOn", descending: true});

    if (!hasItems) {
      let menuitem = document.createElement("menuitem");
      let bundle =
        Services.strings.createBundle("chrome://browser/locale/places/places.properties");
      menuitem.setAttribute("label", bundle.GetStringFromName("bookmarksMenuEmptyFolder"));
      menuitem.setAttribute("class", "bookmark-item");
      menuitem.setAttribute("disabled", true);
      target.insertBefore(menuitem, insertPoint);
    }
  }),

  


  toggleSidebar() {
    if (this.enabled) {
      SidebarUI.toggle(READINGLIST_COMMAND_ID);
    }
  },

  


  receiveMessage(message) {
    switch (message.name) {
      case "ReadingList:GetVisibility": {
        if (message.target.messageManager) {
          message.target.messageManager.sendAsyncMessage("ReadingList:VisibilityStatus",
            { isOpen: this.isSidebarOpen });
        }
        break;
      }

      case "ReadingList:ToggleVisibility": {
        this.toggleSidebar();
        break;
      }

      case "ReadingList:ShowIntro": {
        if (this.enabled && !Preferences.get("browser.readinglist.introShown", false)) {
          Preferences.set("browser.readinglist.introShown", true);
          this.showSidebar();
        }
        break;
      }
    }
  },

  






  onPageProxyStateChanged: Task.async(function* (state) {
    if (!this.toolbarButton) {
      
      return;
    }

    let uri;
    if (this.enabled && state == "valid") {
      uri = gBrowser.currentURI;
      if (uri.schemeIs("about"))
        uri = ReaderMode.getOriginalUrl(uri.spec);
      else if (!uri.schemeIs("http") && !uri.schemeIs("https"))
        uri = null;
    }

    let msg = {topic: "UpdateActiveItem", url: null};
    if (!uri) {
      this.toolbarButton.setAttribute("hidden", true);
      if (this.isSidebarOpen)
        document.getElementById("sidebar").contentWindow.postMessage(msg, "*");
      return;
    }

    let isInList = yield ReadingList.hasItemForURL(uri);

    if (window.closed) {
      
      return;
    }

    if (this.isSidebarOpen) {
      if (isInList)
        msg.url = typeof uri == "string" ? uri : uri.spec;
      document.getElementById("sidebar").contentWindow.postMessage(msg, "*");
    }
    this.setToolbarButtonState(isInList);
  }),

  








  setToolbarButtonState(active) {
    this.toolbarButton.setAttribute("already-added", active);

    let type = (active ? "remove" : "add");
    let tooltip = gNavigatorBundle.getString(`readingList.urlbar.${type}`);
    this.toolbarButton.setAttribute("tooltiptext", tooltip);

    this.toolbarButton.removeAttribute("hidden");
  },

  






  togglePageByBrowser: Task.async(function* (browser) {
    let uri = browser.currentURI;
    if (uri.spec.startsWith("about:reader?"))
      uri = ReaderMode.getOriginalUrl(uri.spec);
    if (!uri)
      return;

    let item = yield ReadingList.itemForURL(uri);
    if (item) {
      yield item.delete();
    } else {
      yield ReadingList.addItemFromBrowser(browser, uri);
    }
  }),

  





  isItemForCurrentBrowser(item) {
    let currentURL = gBrowser.currentURI.spec;
    if (currentURL.startsWith("about:reader?"))
      currentURL = ReaderMode.getOriginalUrl(currentURL);

    if (item.url == currentURL || item.resolvedURL == currentURL) {
      return true;
    }
    return false;
  },

  




  onItemAdded(item) {
    if (!Services.prefs.getBoolPref("browser.readinglist.sidebarEverOpened")) {
      SidebarUI.show("readingListSidebar");
    }
    if (this.isItemForCurrentBrowser(item)) {
      this.setToolbarButtonState(true);
      if (this.isSidebarOpen) {
        let msg = {topic: "UpdateActiveItem", url: item.url};
        document.getElementById("sidebar").contentWindow.postMessage(msg, "*");
      }
    }
  },

  




  onItemDeleted(item) {
    if (this.isItemForCurrentBrowser(item)) {
      this.setToolbarButtonState(false);
    }
  },
};
