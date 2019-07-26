



const Cu = Components.utils;

Cu.import("resource://services-sync/main.js");
Cu.import("resource:///modules/PlacesUIUtils.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm");

let RemoteTabViewer = {
  _tabsList: null,

  init: function () {
    Services.obs.addObserver(this, "weave:service:login:finish", false);
    Services.obs.addObserver(this, "weave:engine:sync:finish", false);

    this._tabsList = document.getElementById("tabsList");

    this.buildList(true);
  },

  uninit: function () {
    Services.obs.removeObserver(this, "weave:service:login:finish");
    Services.obs.removeObserver(this, "weave:engine:sync:finish");
  },

  buildList: function(force) {
    if (!Weave.Service.isLoggedIn || !this._refetchTabs(force))
      return;
    
    

    this._generateTabList();
  },

  createItem: function(attrs) {
    let item = document.createElement("richlistitem");

    
    for (let attr in attrs)
      item.setAttribute(attr, attrs[attr]);

    if (attrs["type"] == "tab")
      item.label = attrs.title != "" ? attrs.title : attrs.url;

    return item;
  },

  filterTabs: function(event) {
    let val = event.target.value.toLowerCase();
    let numTabs = this._tabsList.getRowCount();
    let clientTabs = 0;
    let currentClient = null;
    for (let i = 0;i < numTabs;i++) {
      let item = this._tabsList.getItemAtIndex(i);
      let hide = false;
      if (item.getAttribute("type") == "tab") {
        if (!item.getAttribute("url").toLowerCase().contains(val) && 
            !item.getAttribute("title").toLowerCase().contains(val))
          hide = true;
        else
          clientTabs++;
      }
      else if (item.getAttribute("type") == "client") {
        if (currentClient) {
          if (clientTabs == 0)
            currentClient.hidden = true;
        }
        currentClient = item;
        clientTabs = 0;
      }
      item.hidden = hide;
    }
    if (clientTabs == 0)
      currentClient.hidden = true;
  },

  openSelected: function() {
    let items = this._tabsList.selectedItems;
    let urls = [];
    for (let i = 0;i < items.length;i++) {
      if (items[i].getAttribute("type") == "tab") {
        urls.push(items[i].getAttribute("url"));
        let index = this._tabsList.getIndexOfItem(items[i]);
        this._tabsList.removeItemAt(index);
      }
    }
    if (urls.length) {
      getTopWin().gBrowser.loadTabs(urls);
      this._tabsList.clearSelection();
    }
  },

  bookmarkSingleTab: function() {
    let item = this._tabsList.selectedItems[0];
    let uri = Weave.Utils.makeURI(item.getAttribute("url"));
    let title = item.getAttribute("title");
    PlacesUIUtils.showBookmarkDialog({ action: "add"
                                     , type: "bookmark"
                                     , uri: uri
                                     , title: title
                                     , hiddenRows: [ "description"
                                                   , "location"
                                                   , "loadInSidebar"
                                                   , "keyword" ]
                                     }, window.top);
  },

  bookmarkSelectedTabs: function() {
    let items = this._tabsList.selectedItems;
    let URIs = [];
    for (let i = 0;i < items.length;i++) {
      if (items[i].getAttribute("type") == "tab") {
        let uri = Weave.Utils.makeURI(items[i].getAttribute("url"));
        if (!uri)
          continue;

        URIs.push(uri);
      }
    }
    if (URIs.length) {
      PlacesUIUtils.showBookmarkDialog({ action: "add"
                                       , type: "folder"
                                       , URIList: URIs
                                       , hiddenRows: [ "description" ]
                                       }, window.top);
    }
  },

  getIcon: function (iconUri, defaultIcon) {
    try {
      let iconURI = Weave.Utils.makeURI(iconUri);
      return PlacesUtils.favicons.getFaviconLinkForIcon(iconURI).spec;
    } catch(ex) {
      
    }

    
    return defaultIcon || PlacesUtils.favicons.defaultFavicon.spec;
  },

  _generateTabList: function() {
    let engine = Weave.Service.engineManager.get("tabs");
    let list = this._tabsList;

    
    let count = list.getRowCount();
    if (count > 0) {
      for (let i = count - 1; i >= 0; i--)
        list.removeItemAt(i);
    }

    let seenURLs = new Set();
    let localURLs = engine.getOpenURLs();

    for (let [guid, client] in Iterator(engine.getAllClients())) {
      
      let appendClient = true;

      client.tabs.forEach(function({title, urlHistory, icon}) {
        let url = urlHistory[0];
        if (!url || localURLs.has(url) || seenURLs.has(url)) {
          return;
        }
        seenURLs.add(url);

        if (appendClient) {
          let attrs = {
            type: "client",
            clientName: client.clientName,
            class: Weave.Service.clientsEngine.isMobile(client.id) ? "mobile" : "desktop"
          };
          let clientEnt = this.createItem(attrs);
          list.appendChild(clientEnt);
          appendClient = false;
          clientEnt.disabled = true;
        }
        let attrs = {
          type:  "tab",
          title: title || url,
          url:   url,
          icon:  this.getIcon(icon),
        }
        let tab = this.createItem(attrs);
        list.appendChild(tab);
      }, this);
    }
  },

  adjustContextMenu: function(event) {
    let mode = "all";
    switch (this._tabsList.selectedItems.length) {
      case 0:
        break;
      case 1:
        mode = "single"
        break;
      default:
        mode = "multiple";
        break;
    }
    let menu = document.getElementById("tabListContext");
    let el = menu.firstChild;
    while (el) {
      let showFor = el.getAttribute("showFor");
      if (showFor)
        el.hidden = showFor != mode && showFor != "all";

      el = el.nextSibling;
    }
  },

  _refetchTabs: function(force) {
    if (!force) {
      
      let lastFetch = 0;
      try {
        lastFetch = Services.prefs.getIntPref("services.sync.lastTabFetch");
      }
      catch (e) {  }
      let now = Math.floor(Date.now() / 1000);
      if (now - lastFetch < 30)
        return false;
    }

    
    if (Weave.Service.clientsEngine.lastSync == 0)
      Weave.Service.clientsEngine.sync();

    
    let engine = Weave.Service.engineManager.get("tabs");
    engine.lastModified = null;
    engine.sync();
    Services.prefs.setIntPref("services.sync.lastTabFetch",
                              Math.floor(Date.now() / 1000));

    return true;
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "weave:service:login:finish":
        this.buildList(true);
        break;
      case "weave:engine:sync:finish":
        if (subject == "tabs")
          this._generateTabList();
        break;
    }
  },

  handleClick: function(event) {
    if (event.target.getAttribute("type") != "tab")
      return;

    if (event.button == 1) {
      let url = event.target.getAttribute("url");
      openUILink(url, event);
      let index = this._tabsList.getIndexOfItem(event.target);
      this._tabsList.removeItemAt(index);
    }
  }
}

