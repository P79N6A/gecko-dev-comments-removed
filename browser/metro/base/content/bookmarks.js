



'use strict';




var Bookmarks = {
  get metroRoot() {
    return PlacesUtils.annotations.getItemsWithAnnotation('metro/bookmarksRoot', {})[0];
  },

  logging: false,
  log: function(msg) {
    if (this.logging) {
      Services.console.logStringMessage(msg);
    }
  },

  addForURI: function bh_addForURI(aURI, aTitle, callback) {
    this.isURIBookmarked(aURI, function (isBookmarked) {
      if (isBookmarked)
        return;

      let bookmarkTitle = aTitle || aURI.spec;
      let bookmarkService = PlacesUtils.bookmarks;
      let bookmarkId = bookmarkService.insertBookmark(Bookmarks.metroRoot,
                                                      aURI,
                                                      bookmarkService.DEFAULT_INDEX,
                                                      bookmarkTitle);

      
      let event = document.createEvent("Events");
      event.initEvent("BookmarkCreated", true, false);
      window.dispatchEvent(event);

      if (callback)
        callback(bookmarkId);
    });
  },

  _isMetroBookmark: function(aItemId) {
    return PlacesUtils.bookmarks.getFolderIdForItem(aItemId) == Bookmarks.metroRoot;
  },

  isURIBookmarked: function bh_isURIBookmarked(aURI, callback) {
    if (!callback)
      return;
    PlacesUtils.asyncGetBookmarkIds(aURI, aItemIds => {
      callback(aItemIds && aItemIds.length > 0 && aItemIds.some(this._isMetroBookmark));
    });
  },

  removeForURI: function bh_removeForURI(aURI, callback) {
    
    
    
    PlacesUtils.asyncGetBookmarkIds(aURI, (aItemIds) => {
      aItemIds.forEach((aItemId) => {
        if (this._isMetroBookmark(aItemId)) {
          PlacesUtils.bookmarks.removeItem(aItemId);
        }
      });

      if (callback)
        callback();

      
      let event = document.createEvent("Events");
      event.initEvent("BookmarkRemoved", true, false);
      window.dispatchEvent(event);
    });
  }
};
