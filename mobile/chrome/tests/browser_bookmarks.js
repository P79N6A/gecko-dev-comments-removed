





var testURL_01 = chromeRoot + "browser_blank_01.html";
var testURL_02 = chromeRoot + "browser_blank_02.html";


var gTests = [];
var gCurrentTest = null;



function test() {
  
  
  waitForExplicitFinish();

  
  runNextTest();
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    try {
      PlacesUtils.bookmarks.removeFolderChildren(BookmarkList.panel.mobileRoot);
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Test adding a bookmark with the Star button",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_01, true);

    
    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageReady();
      }
    });
  },

  onPageReady: function() {
    let starbutton = document.getElementById("tool-star");
    starbutton.click();

    let bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_01));
    ok(bookmark != -1, testURL_01 + " should be added.");

    Browser.closeTab(gCurrentTest._currentTab);

    runNextTest();
  }
});



gTests.push({
  desc: "Test clicking on a bookmark loads the web page",
  _currentTab: null,

  run: function() {
    info("is nav panel open: " + BrowserUI.isAutoCompleteOpen())
    BrowserUI.closeAutoComplete(true);
    info("opening new tab")
    this._currentTab = Browser.addTab(testURL_02, true);

    
    messageManager.addMessageListener("pageshow", function(aMessage) {
      info("got a pageshow: " + gCurrentTest._currentTab.browser.currentURI.spec)
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        info("got the right pageshow")
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageReady();
      }
    });
  },

  onPageReady: function() {
    
    window.addEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);
    info("opening nav panel")
    BrowserUI.doCommand("cmd_bookmarks");
  },

  onBookmarksReady: function() {
    info("nav panel is open")
    window.removeEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);

    let bookmarkitem = document.getAnonymousElementByAttribute(BookmarkList.panel, "uri", testURL_01);
    bookmarkitem.control.scrollBoxObject.ensureElementIsVisible(bookmarkitem);

    isnot(bookmarkitem, null, "Found the bookmark");
    is(bookmarkitem.getAttribute("uri"), testURL_01, "Bookmark has the right URL via attribute");
    is(bookmarkitem.spec, testURL_01, "Bookmark has the right URL via property");

    
    messageManager.addMessageListener("pageshow", function(aMessage) {
      messageManager.removeMessageListener(aMessage.name, arguments.callee);
      is(gCurrentTest._currentTab.browser.currentURI.spec, testURL_01, "Opened the right bookmark");

      Browser.closeTab(gCurrentTest._currentTab);

      runNextTest();
    });

    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.width / 2, bookmarkitem.height / 2, {});
  }
});



gTests.push({
  desc: "Test editing URI of existing bookmark",

  run: function() {
    
    window.addEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);
    BrowserUI.doCommand("cmd_bookmarks");
  },

  onBookmarksReady: function() {
    window.removeEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);

    
    let bookmark = BookmarkList.panel.items[0];
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmarkitem = document.getAnonymousElementByAttribute(BookmarkList.panel, "uri", testURL_01);
    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.width / 2, bookmarkitem.height / 2, {});

    let uritextbox = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "uri");
    uritextbox.value = testURL_02;

    let donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();

    let bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_01));
    is(bookmark, -1, testURL_01 + " should no longer in bookmark");
    bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02));
    isnot(bookmark, -1, testURL_02 + " is in bookmark");

    BrowserUI.activePanel = null;

    runNextTest();
  }
});



gTests.push({
  desc: "Test editing title of existing bookmark",

  run: function() {
    
    window.addEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);
    BrowserUI.doCommand("cmd_bookmarks");
  },

  onBookmarksReady: function() {
    window.removeEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);

    
    let bookmark = BookmarkList.panel.items[0];
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02));
    is(PlacesUtils.bookmarks.getItemTitle(bookmark), "Browser Blank Page 01", "Title remains the same.");

    let bookmarkitem = document.getAnonymousElementByAttribute(BookmarkList.panel, "uri", testURL_02);
    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.width / 2, bookmarkitem.height / 2, {});

    let titletextbox = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "name");
    let newtitle = "Changed Title";
    titletextbox.value = newtitle;

    let donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();

    isnot(PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02)), -1, testURL_02 + " is still in bookmark.");
    is(PlacesUtils.bookmarks.getItemTitle(bookmark), newtitle, "Title is changed.");

    BrowserUI.activePanel = null;

    runNextTest();
  }
});



gTests.push({
  desc: "Test removing existing bookmark",
  bookmarkitem: null,

  run: function() {
    
    window.addEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);
    BrowserUI.doCommand("cmd_bookmarks");
  },

  onBookmarksReady: function() {
    window.removeEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);

    
    let bookmark = BookmarkList.panel.items[0];
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmark = document.getAnonymousElementByAttribute(BookmarkList.panel, "uri", testURL_02);
    bookmark.remove();

    let bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02));
    ok(bookmark == -1, testURL_02 + " should no longer in bookmark");
    bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_01));
    ok(bookmark == -1, testURL_01 + " should no longer in bookmark");

    BrowserUI.activePanel = null;

    runNextTest();
  }
});



gTests.push({
  desc: "Test editing title of desktop folder",
  bmId: null,

  run: function() {
    
    gCurrentTest.bmId = PlacesUtils.bookmarks
                                   .insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                   makeURI(testURL_02),
                                                   Ci.nsINavBookmarksService.DEFAULT_INDEX,
                                                   testURL_02);

    
    window.addEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);
    BrowserUI.doCommand("cmd_bookmarks");
  },

  onBookmarksReady: function() {
    window.removeEventListener("NavigationPanelShown", gCurrentTest.onBookmarksReady, false);

    
    let bookmarksPanel = BookmarkList.panel;
    let bookmark = bookmarksPanel.items[0];
    bookmark.startEditing();

    
    let first = bookmarksPanel._children.firstChild;
    is(first.itemId, bookmarksPanel._desktopFolderId, "Desktop folder is showing");

    
    is(first.isEditing, false, "Desktop folder is not in edit mode");

    
    EventUtils.synthesizeMouse(first, first.width / 2, first.height / 2, {});

    
    
    first = bookmarksPanel._children.firstChild;

    
    isnot(first.itemId, bookmarksPanel._desktopFolderId, "Desktop folder is not showing after mouse click");

    
    isnot(bookmarksPanel._readOnlyFolders.indexOf(parseInt(first.itemId)), -1, "Desktop subfolder is showing after mouse click");

    PlacesUtils.bookmarks.removeItem(gCurrentTest.bmId);

    BrowserUI.activePanel = null;
    runNextTest();
  }
});
