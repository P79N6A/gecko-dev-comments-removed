





var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var thread = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager).currentThread;
var testURL_02 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_02.html";
var chromeWindow = window;




















var gTests = [];
var gCurrentTest = null;



function test() {
  
  ok(isnot, "Mochitest must be in context");
  ok(ioService, "nsIIOService must be in context");
  ok(thread, "nsIThreadManager must be in context");
  ok(PlacesUtils, "PlacesUtils must be in context");
  ok(EventUtils, "EventUtils must be in context");
  ok(chromeWindow, "ChromeWindow must be in context");
  
  ok(true, "*** Starting test browser_bookmark_folders.js\n"); 
  runNextTest();
}



function runNextTest() {
  
  if(gCurrentTest) {
    ok(true, "*** FINISHED TEST ***");
  }
  
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    ok(true, gCurrentTest.desc);
    gCurrentTest.run();
    while(!gCurrentTest.isCompleted) {
      thread.processNextEvent(true);
    }
    runNextTest();
  }
  else {
    
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.bookmarks.unfiledBookmarksFolder);
    ok(true, "*** ALL TESTS COMPLETED ***");
  }
}



gTests.push({
  desc: "Test adding folder",
  isCompleted: false,
  
  run: function() {  
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "class", "bookmark-item");
    is(bookmarkitem, null, "folder does not exist yet");
    
    chromeWindow.BrowserUI.showBookmarks();
    chromeWindow.BookmarkList.toggleManage();
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var newfolderbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "class", "bookmark-folder-new");
    EventUtils.synthesizeMouse(newfolderbutton, newfolderbutton.clientWidth / 2, newfolderbutton.clientHeight / 2, {});

    bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "New folder");
    var nametextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "name");
    nametextbox.value = "Test Folder";

    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    is(PlacesUtils.bookmarks.getItemType(bookmarkitem.itemId), PlacesUtils.bookmarks.TYPE_FOLDER, "bookmark item is a folder");
    is(PlacesUtils.bookmarks.getItemTitle(bookmarkitem.itemId), "Test Folder", "folder is created");
    chromeWindow.BookmarkList.close();
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test editing folder",
  isCompleted: false,
  
  run: function() {  
    chromeWindow.BrowserUI.showBookmarks();
    chromeWindow.BookmarkList.toggleManage();
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "Test Folder");
    
    var editbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "edit-button");
    editbutton.click();
    
    var nametextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "name");
    nametextbox.value = "Edited Test Folder";

    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    is(PlacesUtils.bookmarks.getItemTitle(bookmarkitem.itemId), "Edited Test Folder", "folder is successfully edited");
    chromeWindow.BookmarkList.close();
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test removing folder",
  isCompleted: false,
  
  run: function() {  
    chromeWindow.BrowserUI.showBookmarks();
    chromeWindow.BookmarkList.toggleManage();
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "Edited Test Folder");
    var folderid = bookmarkitem.itemId;

    var closeimagebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "close-button");
    
    var handleevent2 = function() {
      closeimagebutton.removeEventListener("load", handleevent2, true);      
      EventUtils.synthesizeMouse(closeimagebutton, closeimagebutton.clientWidth / 2, closeimagebutton.clientHeight / 2, {});
      
      
      
      try {
        var title = PlacesUtils.bookmarks.getItemTitle(folderid);
        ok(false, "folder is not removed"); 
      } catch(error) {
        ok(error.message, "folder is removed, folder ID is not longer valid");
      }
      chromeWindow.BookmarkList.close();
      
      gCurrentTest.isCompleted = true;
    };
    closeimagebutton.addEventListener("load", handleevent2, true);
  },
  
});



gTests.push({
  desc: "Test moving a bookmark into a folder",
  isCompleted: false,
  _currenttab: null,
  
  run: function() {  
    _currenttab = chromeWindow.Browser.addTab(testURL_02, true);
    var handleevent1 = function() {
      _currenttab.browser.removeEventListener("load", handleevent1, true);
      gCurrentTest.verify();
    };
    _currenttab.browser.addEventListener("load", handleevent1 , true);
  },
  
  verify: function() {
  
    
    
    chromeWindow.BrowserUI.doCommand("cmd_star");
    
    chromeWindow.BrowserUI.showBookmarks();
    chromeWindow.BookmarkList.toggleManage();
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var newfolderbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "class", "bookmark-folder-new");
    EventUtils.synthesizeMouse(newfolderbutton, newfolderbutton.clientWidth / 2, newfolderbutton.clientHeight / 2, {});
    var folderitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "New folder");
    var nametextbox = chromeWindow.document.getAnonymousElementByAttribute(folderitem, "anonid", "name");
    nametextbox.value = "Test Folder 1";
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(folderitem, "anonid", "done-button");
    donebutton.click();
    
    var bookmarkitemid = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02));
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "itemid", bookmarkitemid);
    var movebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "folder-button");
    movebutton.click();
    
    var folderitems = chromeWindow.document.getElementById("folder-items");
    var destfolder = chromeWindow.document.getAnonymousElementByAttribute(folderitems, "itemid", folderitem.itemId);
    EventUtils.synthesizeMouse(destfolder, destfolder.clientWidth / 2, destfolder.clientHeight / 2, {});
    
    isnot(PlacesUtils.bookmarks.getFolderIdForItem(bookmarkitemid), PlacesUtils.bookmarks.unfiledBookmarksFolder, 
      "Bookmark is no longer in Bookmarks Menu top level folder");
    is(PlacesUtils.bookmarks.getFolderIdForItem(bookmarkitemid), folderitem.itemId, "Bookmark is moved to a folder");
    
    chromeWindow.BookmarkList.close();
    chromeWindow.Browser.closeTab(_currenttab);
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test moving a folder into a folder",
  isCompleted: false,
  
  run: function() {
  
    
    
    
    chromeWindow.BrowserUI.showBookmarks();
    chromeWindow.BookmarkList.toggleManage();    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    
    var newfolderbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "class", "bookmark-folder-new");
    EventUtils.synthesizeMouse(newfolderbutton, newfolderbutton.clientWidth / 2, newfolderbutton.clientHeight / 2, {});
    var folderitem2 = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "New folder");
    var nametextbox = chromeWindow.document.getAnonymousElementByAttribute(folderitem2, "anonid", "name");
    nametextbox.value = "Test Folder 2";
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(folderitem2, "anonid", "done-button");
    donebutton.click();
    
    var folderitem1 = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "Test Folder 1");
    var foldetitem1id = folderitem1.itemId;
    var movebutton = chromeWindow.document.getAnonymousElementByAttribute(folderitem1, "anonid", "folder-button");
    movebutton.click();
    
    var folderitems = chromeWindow.document.getElementById("folder-items");
    var destfolder = chromeWindow.document.getAnonymousElementByAttribute(folderitems, "itemid", folderitem2.itemId);
    EventUtils.synthesizeMouse(destfolder, destfolder.clientWidth / 2, destfolder.clientHeight / 2, {});
    
    isnot(PlacesUtils.bookmarks.getFolderIdForItem(foldetitem1id), PlacesUtils.bookmarks.unfiledBookmarksFolder, 
      "Folder created in previous test is no longer in Bookmarks Menu top level folder");
    is(PlacesUtils.bookmarks.getFolderIdForItem(foldetitem1id), folderitem2.itemId, "Folder is moved to another folder");
    
    chromeWindow.BookmarkList.close();
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test adding a subfolder into a folder",
  isCompleted: false,
  
  run: function() {
    chromeWindow.BrowserUI.showBookmarks();
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    
    
    
    var folderitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "Test Folder 2");
    var folderitemid = folderitem.itemId;
    EventUtils.synthesizeMouse(folderitem, folderitem.clientWidth / 2, folderitem.clientHeight / 2, {});
    
    bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    chromeWindow.BookmarkList.toggleManage();
    
    var newfolderbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "class", "bookmark-folder-new");
    EventUtils.synthesizeMouse(newfolderbutton, newfolderbutton.clientWidth / 2, newfolderbutton.clientHeight / 2, {});
    
    var newsubfolder = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "title", "New folder");
    var nametextbox = chromeWindow.document.getAnonymousElementByAttribute(newsubfolder, "anonid", "name");
    nametextbox.value = "Test Subfolder 1";
    
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(newsubfolder, "anonid", "done-button");
    donebutton.click();
    
    is(PlacesUtils.bookmarks.getFolderIdForItem(newsubfolder.itemId), folderitemid, "Subfolder is created");
    
    
    
    var editbutton = chromeWindow.document.getAnonymousElementByAttribute(newsubfolder, "anonid", "edit-button");
    editbutton.click();
    
    nametextbox = chromeWindow.document.getAnonymousElementByAttribute(newsubfolder, "anonid", "name");
    nametextbox.value = "Edited Test Subfolder 1";    
    donebutton.click();
    
    is(PlacesUtils.bookmarks.getItemTitle(newsubfolder.itemId), "Edited Test Subfolder 1", "Subfolder is successfully edited");
    
    
    
    var closeimagebutton = chromeWindow.document.getAnonymousElementByAttribute(newsubfolder, "anonid", "close-button");
    var handleevent3 = function() {
      closeimagebutton.removeEventListener("load", handleevent3, true);
      EventUtils.synthesizeMouse(closeimagebutton, closeimagebutton.clientWidth / 2, closeimagebutton.clientHeight / 2, {});
      
      
      
      try {
        var title = PlacesUtils.bookmarks.getItemTitle(newsubfolder.itemId);
        ok(false, "folder is not removed");
      } catch(error) {
        ok(error.message, "Subfolder is removed, folder ID is not longer valid");
      }
      chromeWindow.BookmarkList.close();
      
      gCurrentTest.isCompleted = true;
    };
    closeimagebutton.addEventListener("load", handleevent3, true);
  },
  
});



function uri(spec) {
  return ioService.newURI(spec, null, null);
}
