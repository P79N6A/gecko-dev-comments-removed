



 




var EXPORTED_SYMBOLS = ["PlacesItem", "Bookmark", "Separator", "Livemark",
                        "BookmarkFolder", "DumpBookmarks"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/PlacesBackups.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-common/async.js");
Cu.import("resource://tps/logger.jsm");

var DumpBookmarks = function TPS_Bookmarks__DumpBookmarks() {
  let cb = Async.makeSpinningCallback();
  PlacesBackups.getBookmarksTree().then(result => {
    let [bookmarks, count] = result;
    Logger.logInfo("Dumping Bookmarks...\n" + JSON.stringify(bookmarks) + "\n\n");
    cb(null);
  }).then(null, error => {
    cb(error);
  });
  cb.wait();
};




function extend(child, supertype)
{
   child.prototype.__proto__ = supertype.prototype;
}




function PlacesItemProps(props) {
  this.location = null;
  this.uri = null;
  this.loadInSidebar = null;
  this.keyword = null;
  this.title = null;
  this.description = null;
  this.after = null;
  this.before = null;
  this.folder = null;
  this.position = null;
  this.delete = false;
  this.siteUri = null;
  this.feedUri = null;
  this.livemark = null;
  this.tags = null;
  this.last_item_pos = null;
  this.type = null;

  for (var prop in props) {
    if (prop in this)
      this[prop] = props[prop];
  }
}




function PlacesItem(props) {
  this.props = new PlacesItemProps(props);
  if (this.props.location == null)
    this.props.location = "menu";
  if ("changes" in props)
    this.updateProps = new PlacesItemProps(props.changes);
  else
    this.updateProps = null;
}




PlacesItem.prototype = {
  
  _bookmarkFolders: {
    "places": "placesRoot",
    "menu": "bookmarksMenuFolder",
    "tags": "tagFolder",
    "unfiled": "unfiledBookmarksFolder",
    "toolbar": "toolbarFolder",
  },

  toString: function() {
    var that = this;
    var props = ['uri', 'title', 'location', 'folder', 'feedUri', 'siteUri', 'livemark'];
    var string = (this.props.type ? this.props.type + " " : "") +
      "(" +
      (function() {
        var ret = [];
        for (var i in props) {
          if (that.props[props[i]]) {
            ret.push(props[i] + ": " + that.props[props[i]])
          }
        }
        return ret;
      })().join(", ") + ")";
    return string;
  },

  














  GetPlacesNodeId: function (folder, type, title, uri) {
    let node_id = -1;

    let options = PlacesUtils.history.getNewQueryOptions();
    let query = PlacesUtils.history.getNewQuery();
    query.setFolders([folder], 1);
    let result = PlacesUtils.history.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;

    for (let j = 0; j < rootNode.childCount; j ++) {
      let node = rootNode.getChild(j);
      if (node.title == title) {
        if (type == null || type == undefined || node.type == type)
          if (uri == undefined || uri == null || node.uri.spec == uri.spec)
            node_id = node.itemId;
      }
    }
    rootNode.containerOpen = false;

    return node_id;
  },

  












  IsAdjacentTo: function(itemName, relativePos) {
    Logger.AssertTrue(this.props.folder_id != -1 && this.props.item_id != -1,
      "Either folder_id or item_id was invalid");
    let other_id = this.GetPlacesNodeId(this.props.folder_id, null, itemName);
    Logger.AssertTrue(other_id != -1, "item " + itemName + " not found");
    let other_pos = PlacesUtils.bookmarks.getItemIndex(other_id);
    let this_pos = PlacesUtils.bookmarks.getItemIndex(this.props.item_id);
    if (other_pos + relativePos != this_pos) {
      Logger.logPotentialError("Invalid position - " +
       (this.props.title ? this.props.title : this.props.folder) +
      " not " + (relativePos == 1 ? "after " : "before ") + itemName +
      " for " + this.toString());
      return false;
    }
    return true;
  },

  






  GetItemIndex: function() {
    if (this.props.item_id == -1)
      return -1;
    return PlacesUtils.bookmarks.getItemIndex(this.props.item_id);
  },

  








  GetFolder: function(location) {
    let folder_parts = location.split("/");
    if (!(folder_parts[0] in this._bookmarkFolders)) {
      return -1;
    }
    let folder_id = PlacesUtils.bookmarks[this._bookmarkFolders[folder_parts[0]]];
    for (let i = 1; i < folder_parts.length; i++) {
      let subfolder_id = this.GetPlacesNodeId(
        folder_id,
        Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
        folder_parts[i]);
      if (subfolder_id == -1) {
        return -1;
      }
      else {
        folder_id = subfolder_id;
      }
    }
    return folder_id;
  },

  








  CreateFolder: function(location) {
    let folder_parts = location.split("/");
    if (!(folder_parts[0] in this._bookmarkFolders)) {
      return -1;
    }
    let folder_id = PlacesUtils.bookmarks[this._bookmarkFolders[folder_parts[0]]];
    for (let i = 1; i < folder_parts.length; i++) {
      let subfolder_id = this.GetPlacesNodeId(
        folder_id,
        Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
        folder_parts[i]);
      if (subfolder_id == -1) {
        folder_id = PlacesUtils.bookmarks.createFolder(folder_id,
                                                 folder_parts[i], -1);
      }
      else {
        folder_id = subfolder_id;
      }
    }
    return folder_id;
  },

  








  GetOrCreateFolder: function(location) {
    folder_id = this.GetFolder(location);
    if (folder_id == -1)
      folder_id = this.CreateFolder(location);
    return folder_id;
  },

  










  CheckDescription: function(expectedDescription) {
    if (expectedDescription != null) {
      let description = "";
      if (PlacesUtils.annotations.itemHasAnnotation(this.props.item_id,
          "bookmarkProperties/description")) {
        description = PlacesUtils.annotations.getItemAnnotation(
          this.props.item_id, "bookmarkProperties/description");
      }
      if (description != expectedDescription) {
        Logger.logPotentialError("Invalid description, expected: " +
          expectedDescription + ", actual: " + description + " for " +
          this.toString());
        return false;
      }
    }
    return true;
  },

  












  CheckPosition: function(before, after, last_item_pos) {
    if (after)
      if (!this.IsAdjacentTo(after, 1)) return false;
    if (before)
      if (!this.IsAdjacentTo(before, -1)) return false;
    if (last_item_pos != null && last_item_pos > -1) {
      if (this.GetItemIndex() != last_item_pos + 1) {
        Logger.logPotentialError("Item not found at the expected index, got " +
          this.GetItemIndex() + ", expected " + (last_item_pos + 1) + " for " +
          this.toString());
        return false;
      }
    }
    return true;
  },

  









  SetLocation: function(location) {
    if (location != null) {
      let newfolder_id = this.GetOrCreateFolder(location);
      Logger.AssertTrue(newfolder_id != -1, "Location " + location +
                        " doesn't exist; can't change item's location");
      PlacesUtils.bookmarks.moveItem(this.props.item_id, newfolder_id, -1);
      this.props.folder_id = newfolder_id;
    }
  },

  








  SetDescription: function(description) {
    if (description != null) {
      if (description != "")
        PlacesUtils.annotations.setItemAnnotation(this.props.item_id,
                                      "bookmarkProperties/description",
                                      description,
                                      0,
                                      PlacesUtils.annotations.EXPIRE_NEVER);
      else
        PlacesUtils.annotations.removeItemAnnotation(this.props.item_id,
                                         "bookmarkProperties/description");
    }
  },

  










  SetPosition: function(position) {
    if (position != null) {
      let newposition = -1;
      if (position != -1) {
        newposition = this.GetPlacesNodeId(this.props.folder_id,
                                           null, position);
        Logger.AssertTrue(newposition != -1, "position " + position +
                          " is invalid; unable to change position");
        newposition = PlacesUtils.bookmarks.getItemIndex(newposition);
      }
      PlacesUtils.bookmarks.moveItem(this.props.item_id,
                               this.props.folder_id, newposition);
    }
  },

  






  SetTitle: function(title) {
    if (title != null) {
      PlacesUtils.bookmarks.setItemTitle(this.props.item_id, title);
    }
  },
};




function Bookmark(props) {
  PlacesItem.call(this, props);
  if (this.props.title == null)
    this.props.title = this.props.uri;
  this.props.type = "bookmark";
}




Bookmark.prototype = {
  








  SetKeyword: function(keyword) {
    if (keyword != null)
      PlacesUtils.bookmarks.setKeywordForBookmark(this.props.item_id, keyword);
  },

  









  SetLoadInSidebar: function(loadInSidebar) {
    if (loadInSidebar == true)
      PlacesUtils.annotations.setItemAnnotation(this.props.item_id,
                                    "bookmarkProperties/loadInSidebar",
                                    true,
                                    0,
                                    PlacesUtils.annotations.EXPIRE_NEVER);
    else if (loadInSidebar == false)
      PlacesUtils.annotations.removeItemAnnotation(this.props.item_id,
                                       "bookmarkProperties/loadInSidebar");
  },

  








  SetTitle: function(title) {
    if (title)
      PlacesUtils.bookmarks.setItemTitle(this.props.item_id, title);
  },

  








  SetUri: function(uri) {
    if (uri) {
      let newURI = Services.io.newURI(uri, null, null);
      PlacesUtils.bookmarks.changeBookmarkURI(this.props.item_id, newURI);
    }
  },

  










  SetTags: function(tags) {
    if (tags != null) {
      let URI = Services.io.newURI(this.props.uri, null, null);
      PlacesUtils.tagging.untagURI(URI, null);
      if (tags.length > 0)
        PlacesUtils.tagging.tagURI(URI, tags);
    }
  },

  






  Create: function() {
    this.props.folder_id = this.GetOrCreateFolder(this.props.location);
    Logger.AssertTrue(this.props.folder_id != -1, "Unable to create " +
      "bookmark, error creating folder " + this.props.location);
    let bookmarkURI = Services.io.newURI(this.props.uri, null, null);
    this.props.item_id = PlacesUtils.bookmarks.insertBookmark(this.props.folder_id,
                                                        bookmarkURI,
                                                        -1,
                                                        this.props.title);
    this.SetKeyword(this.props.keyword);
    this.SetDescription(this.props.description);
    this.SetLoadInSidebar(this.props.loadInSidebar);
    this.SetTags(this.props.tags);
    return this.props.item_id;
  },

  







  Update: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Remove");
    this.SetKeyword(this.updateProps.keyword);
    this.SetDescription(this.updateProps.description);
    this.SetLoadInSidebar(this.updateProps.loadInSidebar);
    this.SetTitle(this.updateProps.title);
    this.SetUri(this.updateProps.uri);
    this.SetTags(this.updateProps.tags);
    this.SetLocation(this.updateProps.location);
    this.SetPosition(this.updateProps.position);
  },

  






  Find: function() {
    this.props.folder_id = this.GetFolder(this.props.location);
    if (this.props.folder_id == -1) {
      Logger.logError("Unable to find folder " + this.props.location);
      return -1;
    }
    let bookmarkTitle = this.props.title;
    this.props.item_id = this.GetPlacesNodeId(this.props.folder_id,
                                              null,
                                              bookmarkTitle,
                                              this.props.uri);

    if (this.props.item_id == -1) {
      Logger.logPotentialError(this.toString() + " not found");
      return -1;
    }
    if (!this.CheckDescription(this.props.description))
      return -1;
    if (this.props.keyword != null) {
      let keyword = PlacesUtils.bookmarks.getKeywordForBookmark(this.props.item_id);
      if (keyword != this.props.keyword) {
        Logger.logPotentialError("Incorrect keyword - expected: " +
          this.props.keyword + ", actual: " + keyword +
          " for " + this.toString());
        return -1;
      }
    }
    let loadInSidebar = PlacesUtils.annotations.itemHasAnnotation(
      this.props.item_id,
      "bookmarkProperties/loadInSidebar");
    if (loadInSidebar)
      loadInSidebar = PlacesUtils.annotations.getItemAnnotation(
        this.props.item_id,
        "bookmarkProperties/loadInSidebar");
    if (this.props.loadInSidebar != null &&
        loadInSidebar != this.props.loadInSidebar) {
      Logger.logPotentialError("Incorrect loadInSidebar setting - expected: " +
        this.props.loadInSidebar + ", actual: " + loadInSidebar +
        " for " + this.toString());
      return -1;
    }
    if (this.props.tags != null) {
      try {
        let URI = Services.io.newURI(this.props.uri, null, null);
        let tags = PlacesUtils.tagging.getTagsForURI(URI, {});
        tags.sort();
        this.props.tags.sort();
        if (JSON.stringify(tags) != JSON.stringify(this.props.tags)) {
          Logger.logPotentialError("Wrong tags - expected: " +
            JSON.stringify(this.props.tags) + ", actual: " +
            JSON.stringify(tags) + " for " + this.toString());
          return -1;
        }
      }
      catch (e) {
        Logger.logPotentialError("error processing tags " + e);
        return -1;
      }
    }
    if (!this.CheckPosition(this.props.before,
                            this.props.after,
                            this.props.last_item_pos))
      return -1;
    return this.props.item_id;
  },

  







  Remove: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Remove");
    PlacesUtils.bookmarks.removeItem(this.props.item_id);
  },
};

extend(Bookmark, PlacesItem);




function BookmarkFolder(props) {
  PlacesItem.call(this, props);
  this.props.type = "folder";
}




BookmarkFolder.prototype = {
  






  Create: function() {
    this.props.folder_id = this.GetOrCreateFolder(this.props.location);
    Logger.AssertTrue(this.props.folder_id != -1, "Unable to create " +
      "folder, error creating parent folder " + this.props.location);
    this.props.item_id = PlacesUtils.bookmarks.createFolder(this.props.folder_id,
                                                      this.props.folder,
                                                      -1);
    this.SetDescription(this.props.description);
    return this.props.folder_id;
  },

  







  Find: function() {
    this.props.folder_id = this.GetFolder(this.props.location);
    if (this.props.folder_id == -1) {
      Logger.logError("Unable to find folder " + this.props.location);
      return -1;
    }
    this.props.item_id = this.GetPlacesNodeId(
                              this.props.folder_id,
                              Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
                              this.props.folder);
    if (!this.CheckDescription(this.props.description))
      return -1;
    if (!this.CheckPosition(this.props.before,
                            this.props.after,
                            this.props.last_item_pos))
      return -1;
    return this.props.item_id;
  },

  







  Remove: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Remove");
    PlacesUtils.bookmarks.removeFolderChildren(this.props.item_id);
    PlacesUtils.bookmarks.removeItem(this.props.item_id);
  },

  







  Update: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Update");
    this.SetLocation(this.updateProps.location);
    this.SetPosition(this.updateProps.position);
    this.SetTitle(this.updateProps.folder);
    this.SetDescription(this.updateProps.description);
  },
};

extend(BookmarkFolder, PlacesItem);




function Livemark(props) {
  PlacesItem.call(this, props);
  this.props.type = "livemark";
}




Livemark.prototype = {
  






  Create: function() {
    this.props.folder_id = this.GetOrCreateFolder(this.props.location);
    Logger.AssertTrue(this.props.folder_id != -1, "Unable to create " +
      "folder, error creating parent folder " + this.props.location);
    let siteURI = null;
    if (this.props.siteUri != null)
      siteURI = Services.io.newURI(this.props.siteUri, null, null);
    let livemarkObj = {parentId: this.props.folder_id,
                       title: this.props.livemark,
                       siteURI: siteURI,
                       feedURI: Services.io.newURI(this.props.feedUri, null, null),
                       index: PlacesUtils.bookmarks.DEFAULT_INDEX};

    
    let spinningCb = Async.makeSpinningCallback();

    PlacesUtils.livemarks.addLivemark(livemarkObj).then(
      aLivemark => { spinningCb(null, [Components.results.NS_OK, aLivemark]) },
      () => { spinningCb(null, [Components.results.NS_ERROR_UNEXPECTED, aLivemark]) }
    );

    let [status, livemark] = spinningCb.wait();
    if (!Components.isSuccessCode(status)) {
      throw status;
    }

    this.props.item_id = livemark.id;
    return this.props.item_id;
  },

  







  Find: function() {
    this.props.folder_id = this.GetFolder(this.props.location);
    if (this.props.folder_id == -1) {
      Logger.logError("Unable to find folder " + this.props.location);
      return -1;
    }
    this.props.item_id = this.GetPlacesNodeId(
                              this.props.folder_id,
                              Ci.nsINavHistoryResultNode.RESULT_TYPE_FOLDER,
                              this.props.livemark);
    if (!PlacesUtils.annotations
                    .itemHasAnnotation(this.props.item_id, PlacesUtils.LMANNO_FEEDURI)) {
      Logger.logPotentialError("livemark folder found, but it's just a regular folder, for " +
        this.toString());
      this.props.item_id = -1;
      return -1;
    }
    let feedURI = Services.io.newURI(this.props.feedUri, null, null);
    let lmFeedURISpec =
      PlacesUtils.annotations.getItemAnnotation(this.props.item_id,
                                                PlacesUtils.LMANNO_FEEDURI);
    if (feedURI.spec != lmFeedURISpec) {
      Logger.logPotentialError("livemark feed uri not correct, expected: " +
        this.props.feedUri + ", actual: " + lmFeedURISpec +
        " for " + this.toString());
      return -1;
    }
    if (this.props.siteUri != null) {
      let siteURI = Services.io.newURI(this.props.siteUri, null, null);
      let lmSiteURISpec =
        PlacesUtils.annotations.getItemAnnotation(this.props.item_id,
                                                  PlacesUtils.LMANNO_SITEURI);
      if (siteURI.spec != lmSiteURISpec) {
        Logger.logPotentialError("livemark site uri not correct, expected: " +
        this.props.siteUri + ", actual: " + lmSiteURISpec + " for " +
        this.toString());
        return -1;
      }
    }
    if (!this.CheckPosition(this.props.before,
                            this.props.after,
                            this.props.last_item_pos))
      return -1;
    return this.props.item_id;
  },

  







  Update: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Update");
    this.SetLocation(this.updateProps.location);
    this.SetPosition(this.updateProps.position);
    this.SetTitle(this.updateProps.livemark);
    return true;
  },

  







  Remove: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Remove");
    PlacesUtils.bookmarks.removeItem(this.props.item_id);
  },
};

extend(Livemark, PlacesItem);




function Separator(props) {
  PlacesItem.call(this, props);
  this.props.type = "separator";
}




Separator.prototype = {
  






  Create: function () {
    this.props.folder_id = this.GetOrCreateFolder(this.props.location);
    Logger.AssertTrue(this.props.folder_id != -1, "Unable to create " +
      "folder, error creating parent folder " + this.props.location);
    this.props.item_id = PlacesUtils.bookmarks.insertSeparator(this.props.folder_id,
                                                         -1);
    return this.props.item_id;
  },

  







  Find: function () {
    this.props.folder_id = this.GetFolder(this.props.location);
    if (this.props.folder_id == -1) {
      Logger.logError("Unable to find folder " + this.props.location);
      return -1;
    }
    if (this.props.before == null && this.props.last_item_pos == null) {
      Logger.logPotentialError("Separator requires 'before' attribute if it's the" +
        "first item in the list");
      return -1;
    }
    let expected_pos = -1;
    if (this.props.before) {
      other_id = this.GetPlacesNodeId(this.props.folder_id,
                                      null,
                                      this.props.before);
      if (other_id == -1) {
        Logger.logPotentialError("Can't find places item " + this.props.before +
          " for locating separator");
        return -1;
      }
      expected_pos = PlacesUtils.bookmarks.getItemIndex(other_id) - 1;
    }
    else {
      expected_pos = this.props.last_item_pos + 1;
    }
    this.props.item_id = PlacesUtils.bookmarks.getIdForItemAt(this.props.folder_id,
                                                        expected_pos);
    if (this.props.item_id == -1) {
      Logger.logPotentialError("No separator found at position " + expected_pos);
    }
    else {
      if (PlacesUtils.bookmarks.getItemType(this.props.item_id) !=
          PlacesUtils.bookmarks.TYPE_SEPARATOR) {
        Logger.logPotentialError("Places item at position " + expected_pos +
          " is not a separator");
        return -1;
      }
    }
    return this.props.item_id;
  },

  







  Update: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Update");
    this.SetLocation(this.updateProps.location);
    this.SetPosition(this.updateProps.position);
    return true;
  },

  







  Remove: function() {
    Logger.AssertTrue(this.props.item_id != -1 && this.props.item_id != null,
      "Invalid item_id during Update");
    PlacesUtils.bookmarks.removeItem(this.props.item_id);
  },
};

extend(Separator, PlacesItem);
