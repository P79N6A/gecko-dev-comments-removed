




































var Bookmarks = {
  bookmarks : null,
  panel : null,
  item : null,

  edit : function(aURI) {
    this.bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this.panel = document.getElementById("bookmark_edit");
    this.panel.hidden = false; 

    var bookmarkIDs = this.bookmarks.getBookmarkIdsForURI(aURI, {});
    if (bookmarkIDs.length > 0) {
      this.item = bookmarkIDs[0];
      document.getElementById("bookmark_url").value = aURI.spec;
      document.getElementById("bookmark_name").value = this.bookmarks.getItemTitle(this.item);

      this.panel.openPopup(document.getElementById("tool_star"), "after_end", 0, 0, false, false);
    }
  },

  remove : function() {
    if (this.item) {
      this.bookmarks.removeItem(this.item);
      document.getElementById("tool_star").removeAttribute("starred");
    }
    this.close();
  },

  save : function() {
    if (this.panel && this.item) {
      var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
      var bookmarkURI = ios.newURI(document.getElementById("bookmark_url").value, null, null);
      if (bookmarkURI) {
        this.bookmarks.setItemTitle(this.item, document.getElementById("bookmark_name").value);
        this.bookmarks.changeBookmarkURI(this.item, bookmarkURI);
      }
    }
    this.close();
  },

  close : function() {
    if (this.panel) {
      this.item = null;
      this.panel.hidePopup();
      this.panel = null;
    }
  },

  list : function() {
    this.bookmarks = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
    this.panel = document.getElementById("bookmark_picker");
    this.panel.hidden = false; 

    var list = document.getElementById("bookmark_list");
    while (list.childNodes.length > 0) {
      list.removeChild(list.childNodes[0]);
    }

    var fis = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);

    var items = this.getBookmarks();
    if (items.length > 0) {
      for (var i=0; i<items.length; i++) {
        var itemId = items[i];
        var listItem = document.createElement("richlistitem");
        listItem.setAttribute("class", "bookmarklist-item");

        var box = document.createElement("vbox");
        box.setAttribute("pack", "center");
        var image = document.createElement("image");
        image.setAttribute("class", "bookmarklist-image");
        image.setAttribute("src", fis.getFaviconImageForPage(this.bookmarks.getBookmarkURI(itemId)).spec);
        box.appendChild(image);
        listItem.appendChild(box);

        var label = document.createElement("label");
        label.setAttribute("class", "bookmarklist-text");
        label.setAttribute("value", this.bookmarks.getItemTitle(itemId));
        label.setAttribute("flex", "1");
        label.setAttribute("crop", "end");
        label.setAttribute("onclick", "Bookmarks.open(" + itemId + ");");
        listItem.appendChild(label);
        list.appendChild(listItem);
      }
      this.panel.openPopup(document.getElementById("tool_bookmarks"), "after_end", 0, 0, false, false);
    }
  },

  open : function(aItem) {
    var bookmarkURI = this.bookmarks.getBookmarkURI(aItem);
    getBrowser().loadURI(bookmarkURI.spec, null, null, false);
    this.close();
  },

  getBookmarks : function() {
    var items = [];

    var history = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
    var options = history.getNewQueryOptions();
    var query = history.getNewQuery();
    query.setFolders([this.bookmarks.bookmarksMenuFolder], 1);
    var result = history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i<cc; ++i) {
      var node = rootNode.getChild(i);
      if (node.type == node.RESULT_TYPE_URI) {
        items.push(node.itemId);
      }
    }
    rootNode.containerOpen = false;

    return items;
  }
};
