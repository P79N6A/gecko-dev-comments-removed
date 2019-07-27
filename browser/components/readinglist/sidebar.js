



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/readinglist/ReadingList.jsm");

let log = Cu.import("resource://gre/modules/Log.jsm", {})
            .Log.repository.getLogger("readinglist.sidebar");


let RLSidebar = {
  



  list: null,

  



  listPromise: null,

  



  itemTemplate: null,

  



  itemsById: new Map(),
  



  itemNodesById: new Map(),

  


  init() {
    log.debug("Initializing");

    addEventListener("unload", () => this.uninit());

    this.list = document.getElementById("list");
    this.emptyListInfo = document.getElementById("emptyListInfo");
    this.itemTemplate = document.getElementById("item-template");

    
    document.addEventListener("click", event => this.onClick(event));

    this.list.addEventListener("mousemove", event => this.onListMouseMove(event));
    this.list.addEventListener("keydown", event => this.onListKeyDown(event), true);

    window.addEventListener("message", event => this.onMessage(event));

    this.listPromise = this.ensureListItems();
    ReadingList.addListener(this);

    Services.prefs.setBoolPref("browser.readinglist.sidebarEverOpened", true);

    let initEvent = new CustomEvent("Initialized", {bubbles: true});
    document.documentElement.dispatchEvent(initEvent);
  },

  


  uninit() {
    log.debug("Shutting down");

    ReadingList.removeListener(this);
  },

  






  onItemAdded(item, append = false) {
    log.trace(`onItemAdded: ${item}`);

    let itemNode = document.importNode(this.itemTemplate.content, true).firstElementChild;
    this.updateItem(item, itemNode);
    
    
    if (append)
      this.list.appendChild(itemNode);
    else
      this.list.insertBefore(itemNode, this.list.firstChild);
    this.itemNodesById.set(item.id, itemNode);
    this.itemsById.set(item.id, item);

    this.emptyListInfo.hidden = true;
    window.requestAnimationFrame(() => {
      window.requestAnimationFrame(() => {
        itemNode.classList.add('visible');
      });
    });
  },

  



  onItemDeleted(item) {
    log.trace(`onItemDeleted: ${item}`);

    let itemNode = this.itemNodesById.get(item.id);

    this.itemNodesById.delete(item.id);
    this.itemsById.delete(item.id);

    itemNode.addEventListener('transitionend', (event) => {
      if (event.propertyName == "max-height") {
        itemNode.remove();

        
        

        this.emptyListInfo.hidden = (this.numItems > 0);
      }
    }, false);

    itemNode.classList.remove('visible');
  },

  



  onItemUpdated(item) {
    log.trace(`onItemUpdated: ${item}`);

    let itemNode = this.itemNodesById.get(item.id);
    if (!itemNode)
      return;

    this.updateItem(item, itemNode);
  },

  





  updateItem(item, itemNode) {
    itemNode.setAttribute("id", "item-" + item.id);
    itemNode.setAttribute("title", `${item.title}\n${item.url}`);

    itemNode.querySelector(".item-title").textContent = item.title;

    let domain = item.uri.spec;
    try {
      domain = item.uri.host;
    }
    catch (err) {}
    itemNode.querySelector(".item-domain").textContent = domain;

    let thumb = itemNode.querySelector(".item-thumb-container");
    if (item.preview) {
      thumb.style.backgroundImage = "url(" + item.preview + ")";
    } else {
      thumb.style.removeProperty("background-image");
    }
    thumb.classList.toggle("preview-available", !!item.preview);
  },

  


  ensureListItems: Task.async(function* () {
    yield ReadingList.forEachItem(item => {
      
      try {
        this.onItemAdded(item, true);
      } catch (e) {
        log.warn("Error adding item", e);
      }
    }, {sort: "addedOn", descending: true});
    this.emptyListInfo.hidden = (this.numItems > 0);
  }),

  



  get numItems() {
    return this.list.childElementCount;
  },

  



  get activeItem() {
    return document.querySelector("#list > .item.active");
  },

  set activeItem(node) {
    if (node && node.parentNode != this.list) {
      log.error(`Unable to set activeItem to invalid node ${node}`);
      return;
    }

    log.trace(`Setting activeItem: ${node ? node.id : null}`);

    if (node && node.classList.contains("active")) {
      return;
    }

    let prevItem = document.querySelector("#list > .item.active");
    if (prevItem) {
      prevItem.classList.remove("active");
    }

    if (node) {
      node.classList.add("active");
    }

    let event = new CustomEvent("ActiveItemChanged", {bubbles: true});
    this.list.dispatchEvent(event);
  },

  



  get selectedItem() {
    return document.querySelector("#list > .item.selected");
  },

  set selectedItem(node) {
    if (node && node.parentNode != this.list) {
      log.error(`Unable to set selectedItem to invalid node ${node}`);
      return;
    }

    log.trace(`Setting selectedItem: ${node ? node.id : null}`);

    let prevItem = document.querySelector("#list > .item.selected");
    if (prevItem) {
      prevItem.classList.remove("selected");
    }

    if (node) {
      node.classList.add("selected");
      let itemId = this.getItemIdFromNode(node);
      this.list.setAttribute("aria-activedescendant", "item-" + itemId);
    } else {
      this.list.removeAttribute("aria-activedescendant");
    }

    let event = new CustomEvent("SelectedItemChanged", {bubbles: true});
    this.list.dispatchEvent(event);
  },

  



  get selectedIndex() {
    for (let i = 0; i < this.numItems; i++) {
      let item = this.list.children.item(i);
      if (!item) {
        break;
      }
      if (item.classList.contains("selected")) {
        return i;
      }
    }
    return -1;
  },

  set selectedIndex(index) {
    log.trace(`Setting selectedIndex: ${index}`);

    if (index == -1) {
      this.selectedItem = null;
      return;
    }

    let item = this.list.children.item(index);
    if (!item) {
      log.warn(`Unable to set selectedIndex to invalid index ${index}`);
      return;
    }
    this.selectedItem = item;
  },

  





  openURL(url, event) {
    log.debug(`Opening page ${url}`);

    let mainWindow = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShellTreeItem)
                           .rootTreeItem
                           .QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindow);

    let currentUrl = mainWindow.gBrowser.currentURI.spec;
    if (currentUrl.startsWith("about:reader"))
      url = "about:reader?url=" + encodeURIComponent(url);

    mainWindow.openUILink(url, event);
  },

  




  getItemIdFromNode(node) {
    let id = node.getAttribute("id");
    if (id && id.startsWith("item-")) {
      return id.slice(5);
    }

    return null;
  },

  




  getItemFromNode(node) {
    let itemId = this.getItemIdFromNode(node);
    if (!itemId) {
      return null;
    }

    return this.itemsById.get(itemId);
  },

  



  openActiveItem(event) {
    let itemNode = this.activeItem;
    if (!itemNode) {
      return;
    }

    let item = this.getItemFromNode(itemNode);
    this.openURL(item.url, event);
  },

  




  findParentItemNode(node) {
    while (node && node != this.list && node != document.documentElement &&
           !node.classList.contains("item")) {
      node = node.parentNode;
    }

    if (node != this.list && node != document.documentElement) {
      return node;
    }

    return null;
  },

  



  onClick(event) {
    let itemNode = this.findParentItemNode(event.target);
    if (!itemNode)
      return;

    if (event.target.classList.contains("remove-button")) {
      ReadingList.deleteItem(this.getItemFromNode(itemNode));
      return;
    }

    this.activeItem = itemNode;
    this.openActiveItem(event);
  },

  




  onListMouseMove(event) {
    let itemNode = this.findParentItemNode(event.target);
    if (itemNode != this.selectedItem)
      this.selectedItem = null;
  },

  



  onListKeyDown(event) {
    if (event.keyCode == KeyEvent.DOM_VK_DOWN) {
      
      
      event.preventDefault();

      if (!this.numItems) {
        return;
      }
      let index = this.selectedIndex + 1;
      if (index >= this.numItems) {
        index = 0;
      }

      this.selectedIndex = index;
      this.selectedItem.focus();
    } else if (event.keyCode == KeyEvent.DOM_VK_UP) {
      event.preventDefault();

      if (!this.numItems) {
        return;
      }
      let index = this.selectedIndex - 1;
      if (index < 0) {
        index = this.numItems - 1;
      }

      this.selectedIndex = index;
      this.selectedItem.focus();
    } else if (event.keyCode == KeyEvent.DOM_VK_RETURN) {
      let selectedItem = this.selectedItem;
      if (selectedItem) {
        this.activeItem = selectedItem;
        this.openActiveItem(event);
      }
    }
  },

  



  onMessage(event) {
    let msg = event.data;

    if (msg.topic != "UpdateActiveItem") {
      return;
    }

    if (!msg.url) {
      this.activeItem = null;
    } else {
      ReadingList.itemForURL(msg.url).then(item => {
        let node;
        if (item && (node = this.itemNodesById.get(item.id))) {
          this.activeItem = node;
        }
      });
    }
  }
};


addEventListener("DOMContentLoaded", () => RLSidebar.init());
