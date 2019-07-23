









































function Startup()
{
  const windowNode = document.getElementById("bookmark-window");
  const bookmarksView = document.getElementById("bookmarks-view");

  var titleString;

  
  if ("arguments" in window && window.arguments[0]) {
    var title;
    var uri = window.arguments[0];
    bookmarksView.tree.setAttribute("ref", uri);
    document.getElementById("bookmarks-search").setAttribute("hidden","true")
    if (uri.substring(0,5) == "find:") {
      title = BookmarksUtils.getLocaleString("search_results_title");
      
      
      windowNode.setAttribute("windowtype", "bookmarks:searchresults");
    }
    else
      title = BookmarksUtils.getProperty(window.arguments[0], NC_NS+"Name");
    
    titleString = BookmarksUtils.getLocaleString("window_title", title);
  }
  else {
    titleString = BookmarksUtils.getLocaleString("bookmarks_title", title);
    
    if (!bookmarksView.treeBoxObject.view.isContainerOpen(0))
      bookmarksView.treeBoxObject.view.toggleOpenState(0);
  }

  bookmarksView.treeBoxObject.view.selection.select(0);

  document.title = titleString;

  
  var key = document.getElementById("key_delete");
  if (key.getAttribute("command"))
    key.setAttribute("command", "cmd_bm_delete");
  key = document.getElementById("key_delete2");
  if (key.getAttribute("command"))
    key.setAttribute("command", "cmd_bm_delete");
  
  document.getElementById("CommandUpdate_Bookmarks").setAttribute("commandupdater","true");
  bookmarksView.tree.focus();
}

function Shutdown()
{
  
  var win = document.getElementById("bookmark-window");
  win.setAttribute("x", screenX);
  win.setAttribute("y", screenY);
  win.setAttribute("height", outerHeight);
  win.setAttribute("width", outerWidth);
}

var gConstructedColumnsMenuItems = false;
function fillColumnsMenu(aEvent) 
{
  var bookmarksView = document.getElementById("bookmarks-view");
  var columns = bookmarksView.columns;
  var i;

  if (!gConstructedColumnsMenuItems) {
    for (i = 0; i < columns.length; ++i) {
      var menuitem = document.createElement("menuitem");
      menuitem.setAttribute("label", columns[i].label);
      menuitem.setAttribute("colid", columns[i].id);
      menuitem.setAttribute("id", "columnMenuItem:" + columns[i].id);
      menuitem.setAttribute("type", "checkbox");
      menuitem.setAttribute("checked", columns[i].hidden != "true");
      
      if (columns[i].id == "Name")
        menuitem.setAttribute("disabled", "true");
      aEvent.target.appendChild(menuitem);
    }

    gConstructedColumnsMenuItems = true;
  }
  else {
    for (i = 0; i < columns.length; ++i) {
      var element = document.getElementById("columnMenuItem:" + columns[i].id);
      if (element)
        if (columns[i].hidden == "true")
          element.setAttribute("checked", "false");
        else
          element.setAttribute("checked", "true");
    }
  }
  
  aEvent.stopPropagation();
}

function onViewMenuColumnItemSelected(aEvent)
{
  var colid = aEvent.target.getAttribute("colid");
  if (colid != "") {
    var bookmarksView = document.getElementById("bookmarks-view");
    bookmarksView.toggleColumnVisibility(colid);
  }  

  aEvent.stopPropagation();
}

function OpenBookmarksFile()
{
  const nsIFilePicker = Components.interfaces.nsIFilePicker;
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  fp.init(window, BookmarksUtils.getLocaleString("SelectOpen"), nsIFilePicker.modeOpen);
  fp.appendFilters(nsIFilePicker.filterHTML);
  if (fp.show() == nsIFilePicker.returnOK) {
    var path = Components.classes["@mozilla.org/supports-string;1"]
               .createInstance(Components.interfaces.nsISupportsString);
    path.data = fp.file.path;
    PREF.setComplexValue("browser.bookmarks.file",
                         Components.interfaces.nsISupportsString, path);
  }
}
