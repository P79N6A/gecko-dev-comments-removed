





































var gOKButton;
var gSearchField;
function Startup()
{
  initServices();
  initBMService();
  gOKButton = document.documentElement.getButton("accept");
  gOKButton.disabled = true;
  gSearchField = document.getElementById("searchField");
  gSearchField.focus();
}
var gCreatingNewWindow = false;
function find()
{
  
  
  var match = document.getElementById("matchList");
  var method = document.getElementById("methodList");
  var searchURI = "find:datasource=rdf:bookmarks"
  searchURI += "&match=" + match.selectedItem.value;
  searchURI += "&method=" + method.selectedItem.value;
  searchURI += "&text=" + encodeURIComponent(gSearchField.value);
  var bmWindow = findMostRecentWindow("bookmarks:searchresults", "chrome://communicator/content/bookmarks/bookmarksManager.xul", searchURI);
  
  
  if (!gCreatingNewWindow) 
    bmWindow.document.getElementById("bookmarks-view").tree.setAttribute("ref", searchURI);
 
  bmWindow.focus();

  if (document.getElementById("saveQuery").checked == true)
  {
    var bundle = document.getElementById("bookmarksBundle");
    var findTitle = BookmarksUtils.getLocaleString("ShortFindTitle", [gSearchField.value]);
    BMSVC.addBookmarkImmediately(searchURI, findTitle, BMSVC.BOOKMARK_FIND_TYPE, null);
  }

  return true;
}

function findMostRecentWindow(aType, aURI, aParam)
{
  var topWindow = WINDOWSVC && WINDOWSVC.getMostRecentWindow(aType);
  if (!topWindow) gCreatingNewWindow = true;
  return topWindow || openDialog("chrome://communicator/content/bookmarks/bookmarksManager.xul", 
                                 "", "chrome,all,dialog=no", aParam);
}

function doEnabling()
{
  gOKButton.disabled = !gSearchField.value;
}
