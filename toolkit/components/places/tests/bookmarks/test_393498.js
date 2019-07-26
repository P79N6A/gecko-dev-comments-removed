





let observer = {
  __proto__: NavBookmarkObserver.prototype,

  onItemAdded: function (id, folder, index) {
    this._itemAddedId = id;
    this._itemAddedParent = folder;
    this._itemAddedIndex = index;
  },
  onItemChanged: function (id, property, isAnnotationProperty, value) {
    this._itemChangedId = id;
    this._itemChangedProperty = property;
    this._itemChanged_isAnnotationProperty = isAnnotationProperty;
    this._itemChangedValue = value;
  }
};
PlacesUtils.bookmarks.addObserver(observer, false);

do_register_cleanup(function () {
  PlacesUtils.bookmarks.removeObserver(observer);
});

function run_test() {
  
  
  
  const PAST_PRTIME = (Date.now() - 86400000) * 1000;

  
  let testFolder = PlacesUtils.bookmarks.createFolder(
    PlacesUtils.placesRootId, "test Folder",
    PlacesUtils.bookmarks.DEFAULT_INDEX);
  let bookmarkId = PlacesUtils.bookmarks.insertBookmark(
    testFolder, uri("http://google.com/"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, "");

  
  do_check_true(observer.itemChangedProperty === undefined);

  
  PlacesUtils.bookmarks.setItemDateAdded(bookmarkId, PAST_PRTIME);
  do_check_eq(observer._itemChangedProperty, "dateAdded");
  do_check_eq(observer._itemChangedValue, PAST_PRTIME);
  let dateAdded = PlacesUtils.bookmarks.getItemDateAdded(bookmarkId);
  do_check_eq(dateAdded, PAST_PRTIME);

  
  do_check_eq(PlacesUtils.bookmarks.getItemLastModified(bookmarkId), dateAdded);

  
  PlacesUtils.bookmarks.setItemLastModified(bookmarkId, PAST_PRTIME);
  do_check_eq(observer._itemChangedProperty, "lastModified");
  do_check_eq(observer._itemChangedValue, PAST_PRTIME);
  do_check_eq(PlacesUtils.bookmarks.getItemLastModified(bookmarkId),
              PAST_PRTIME);

  
  PlacesUtils.bookmarks.setItemTitle(bookmarkId, "Google");

  
  do_check_eq(observer._itemChangedId, bookmarkId);
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Google");

  
  is_time_ordered(PAST_PRTIME,
                  PlacesUtils.bookmarks.getItemLastModified(bookmarkId));

  
  let root = PlacesUtils.getFolderContents(testFolder).root;
  do_check_eq(root.childCount, 1);
  let childNode = root.getChild(0);

  
  do_check_eq(PlacesUtils.bookmarks.getItemDateAdded(bookmarkId),
              childNode.dateAdded);
  do_check_eq(PlacesUtils.bookmarks.getItemLastModified(bookmarkId),
              childNode.lastModified);

  
  PlacesUtils.bookmarks.setItemLastModified(bookmarkId, PAST_PRTIME);
  PlacesUtils.bookmarks.setItemTitle(bookmarkId, "Google");

  
  is_time_ordered(PAST_PRTIME, childNode.lastModified);
  
  do_check_eq(PlacesUtils.bookmarks.getItemLastModified(bookmarkId),
              childNode.lastModified);

  
  PlacesUtils.bookmarks.setItemDateAdded(bookmarkId, PAST_PRTIME);
  do_check_eq(childNode.dateAdded, PAST_PRTIME);
  PlacesUtils.bookmarks.setItemLastModified(bookmarkId, PAST_PRTIME);
  do_check_eq(childNode.lastModified, PAST_PRTIME);

  root.containerOpen = false;
}
