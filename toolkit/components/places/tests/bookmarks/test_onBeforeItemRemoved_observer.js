














































let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);




function Observer(aExpectedId)
{
}
Observer.prototype =
{
  checked: false,
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onItemAdded: function(id, folder, index) {
  },
  onBeforeItemRemoved: function(id) {
    this.removedId = id;
  },
  onItemRemoved: function(id, folder, index) {
    do_check_false(this.checked);
    do_check_eq(this.removedId, id);
    this.checked = true;
  },
  onItemChanged: function(id, property, isAnnotationProperty, value) {
  },
  onItemVisited: function(id, visitID, time) {
  },
  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex) {
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};




function test_removeItem()
{
  
  let id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                             uri("http://mozilla.org"), bs.DEFAULT_INDEX, "t");

  
  let observer = new Observer(id);
  bs.addObserver(observer, false);
  bs.removeItem(id);

  
  do_check_true(observer.checked);
  bs.removeObserver(observer);
}

function test_removeFolder()
{
  
  let id = bs.createFolder(bs.unfiledBookmarksFolder, "t", bs.DEFAULT_INDEX);

  
  let observer = new Observer(id);
  bs.addObserver(observer, false);
  bs.removeItem(id);

  
  do_check_true(observer.checked);
  bs.removeObserver(observer);
}

function test_removeFolderChildren()
{
  
  let fid = bs.createFolder(bs.unfiledBookmarksFolder, "tf", bs.DEFAULT_INDEX);
  let id = bs.insertBookmark(fid, uri("http://mozilla.org"), bs.DEFAULT_INDEX,
                             "t");

  
  let observer = new Observer(id);
  bs.addObserver(observer, false);
  bs.removeFolderChildren(fid);

  
  do_check_true(observer.checked);
  bs.removeObserver(observer);
}

function test_setItemIndex()
{
  
  let id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                             uri("http://mozilla.org/1"), 0, "t1");
  bs.insertBookmark(bs.unfiledBookmarksFolder, uri("http://mozilla.org/2"), 1,
                    "t1");

  
  let observer = new Observer(id);
  bs.addObserver(observer, false);
  bs.setItemIndex(id, 2);

  
  do_check_true(observer.checked);
  bs.removeObserver(observer);
}




let tests = [
  test_removeItem,
  test_removeFolder,
  test_removeFolderChildren,
  test_setItemIndex,
];
function run_test()
{
  tests.forEach(function(test) test());
}
