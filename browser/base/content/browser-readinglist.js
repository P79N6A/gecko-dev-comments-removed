





XPCOMUtils.defineLazyModuleGetter(this, "ReadingList",
  "resource:///modules/readinglist/ReadingList.jsm");

const READINGLIST_COMMAND_ID = "readingListSidebar";

let ReadingListUI = {
  MESSAGES: [
    "ReadingList:GetVisibility",
    "ReadingList:ToggleVisibility",
  ],

  


  init() {
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
  },

  



  get enabled() {
    return Preferences.get("browser.readinglist.enabled", false);
  },

  



  get isSidebarOpen() {
    return SidebarUI.isOpen && SidebarUI.currentID == READINGLIST_COMMAND_ID;
  },

  



  updateUI() {
    let enabled = this.enabled;
    if (!enabled) {
      this.hideSidebar();
    }

    document.getElementById(READINGLIST_COMMAND_ID).setAttribute("hidden", !enabled);
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

  onReadingListPopupShowing(target) {
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

    ReadingList.getItems().then(items => {
      for (let item of items) {
        let menuitem = document.createElement("menuitem");
        menuitem.setAttribute("label", item.title || item.url.spec);
        menuitem.setAttribute("class", classList);

        let node = menuitem._placesNode = {
          
          
          type: Ci.nsINavHistoryResultNode.RESULT_TYPE_URI,

          
          
          parent: {type: Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER},

          
          
          
          
          itemId: -1,

          
          uri: item.url.spec,

          
          title: item.title
        };

        Favicons.getFaviconURLForPage(item.url, uri => {
          if (uri) {
            menuitem.setAttribute("image",
                                  Favicons.getFaviconLinkForIcon(uri).spec);
          }
        });

        target.insertBefore(menuitem, insertPoint);
      }

      if (!items.length) {
        let menuitem = document.createElement("menuitem");
        let bundle =
          Services.strings.createBundle("chrome://browser/locale/places/places.properties");
        menuitem.setAttribute("label", bundle.GetStringFromName("bookmarksMenuEmptyFolder"));
        menuitem.setAttribute("class", "bookmark-item");
        menuitem.setAttribute("disabled", true);
        target.insertBefore(menuitem, insertPoint);
      }
    });
  },

  


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
    }
  },
};
