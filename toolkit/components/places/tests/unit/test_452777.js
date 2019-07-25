











































var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

function run_test()
{
  const TITLE = "test folder";

  
  
  
  let id = bs.createFolder(bs.placesRoot, TITLE, -1);
  bs.createFolder(bs.placesRoot, "test folder 2", -1);
  let transaction = bs.getRemoveFolderTransaction(id);
  transaction.doTransaction();

  
  bs.addObserver({
    onItemAdded: function(aItemId, aFolder, aIndex, aItemType, aURI, aTitle)
    {
      do_check_eq(aItemId, id);
      do_check_eq(aTitle, TITLE);
    }
  }, false);
  transaction.undoTransaction();
}
