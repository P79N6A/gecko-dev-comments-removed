









































function Startup() 
{
  
  var key = document.getElementById("key_delete");
  if (key.getAttribute("command"))
    key.setAttribute("command", "cmd_bm_delete");
  key = document.getElementById("key_delete2");
  if (key.getAttribute("command"))
    key.setAttribute("command", "cmd_bm_delete");

  var bookmarksView = document.getElementById("bookmarks-view");  
  bookmarksView.tree.view.selection.select(0);
}

function manageBookmarks() {
  openDialog("chrome://communicator/content/bookmarks/bookmarksManager.xul", "", "chrome,dialog=no,resizable=yes");
}

function addBookmark() {
  var contentArea = top.document.getElementById('content');                   
  if (contentArea) {
    const browsers = contentArea.browsers;
    if (browsers.length > 1)
      BookmarksUtils.addBookmarkForTabBrowser(contentArea);
    else
      BookmarksUtils.addBookmarkForBrowser(contentArea.webNavigation, true);    
  }
  else
    BookmarksUtils.addBookmark(null, null, undefined, true);
}
