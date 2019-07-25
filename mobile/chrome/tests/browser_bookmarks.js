




 
var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService); 
var testURL_01 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
var testURL_02 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_02.html";


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
      PlacesUtils.bookmarks.removeFolderChildren(BookmarkList.mobileRoot);
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Test adding a bookmark with the Star button",
  _currenttab: null,

  run: function() {
    this._currenttab = Browser.addTab(testURL_01, true);

    
    this._currenttab.browser.addEventListener("load", 
    function() {
      gCurrentTest._currenttab.browser.removeEventListener("load", arguments.callee, true);
      gCurrentTest.onPageReady();
    }, 
    true);
  },
  
  onPageReady: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();    
    
    var bookmarkItem = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01));
    ok(bookmarkItem != -1, testURL_01 + " should be added.");

    Browser.closeTab(gCurrentTest._currenttab);
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Test clicking on a bookmark loads the web page",
  _currenttab: null,

  run: function() {
    this._currenttab = Browser.addTab("about:blank", true);

    
    this._currenttab.browser.addEventListener("load", 
    function() {
      gCurrentTest._currenttab.browser.removeEventListener("load", arguments.callee, true);
      gCurrentTest.onPageReady();
    }, 
    true);
  },

  onPageReady: function() {
    
    BookmarkList.show();

    waitFor(gCurrentTest.onBookmarksReady, function() { return document.getElementById("bookmarklist-container").hidden == false; });
  },
  
  onBookmarksReady: function() {
    
    gCurrentTest._currenttab.browser.addEventListener("pageshow", 
      function() {
        gCurrentTest._currenttab.browser.removeEventListener("pageshow", arguments.callee, true);
        todo_is(gCurrentTest._currenttab.browser.currentURI.spec, testURL_01, "Opened the right bookmark");      

        Browser.closeTab(gCurrentTest._currenttab);

        runNextTest();
      }, 
      true);

    var bookmarkitems = document.getElementById("bookmark-items");    
    var bookmarkitem = document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_01);
    isnot(bookmarkitem, null, "Found the bookmark");
    is(bookmarkitem.getAttribute("uri"), testURL_01, "Bookmark has the right URL via attribute");
    is(bookmarkitem.spec, testURL_01, "Bookmark has the right URL via property");

    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.clientWidth / 2, bookmarkitem.clientHeight / 2, {});
  }  
});



gTests.push({
  desc: "Test editing URI of existing bookmark",

  run: function() {
    
    BookmarkList.show();

    
    BookmarkList.toggleManage();

    waitFor(gCurrentTest.onBookmarksReady, function() { return document.getElementById("bookmark-items").manageUI == true; });
  },
  
  onBookmarksReady: function() {
    var bookmarkitems = document.getElementById("bookmark-items");
    var bookmarkitem = document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_01);
    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.clientWidth / 2, bookmarkitem.clientHeight / 2, {});

    var uritextbox = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "uri");
    uritextbox.value = testURL_02;

    var donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();

    var bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01));
    is(bookmark, -1, testURL_01 + " should no longer in bookmark");
    bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02));
    isnot(bookmark, -1, testURL_02 + " is in bookmark");
    
    BookmarkList.close();
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Test editing title of existing bookmark",
  
  run: function() {
    
    BookmarkList.show();

    
    BookmarkList.toggleManage();

    waitFor(gCurrentTest.onBookmarksReady, function() { return document.getElementById("bookmark-items").manageUI == true; });
  },
  
  onBookmarksReady: function() {
    var bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02));
    is(PlacesUtils.bookmarks.getItemTitle(bookmark), "Browser Blank Page 01", "Title remains the same.");
    
    var bookmarkitems = document.getElementById("bookmark-items");
    var bookmarkitem = document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_02);
    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.clientWidth / 2, bookmarkitem.clientHeight / 2, {});
    
    var titletextbox = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "name");
    var newtitle = "Changed Title";
    titletextbox.value = newtitle;
    
    var donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();

    isnot(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02)), -1, testURL_02 + " is still in bookmark.");
    is(PlacesUtils.bookmarks.getItemTitle(bookmark), newtitle, "Title is changed.");
    
    BookmarkList.close();
    
    runNextTest();
  }
});



gTests.push({
  desc: "Test removing existing bookmark",
  bookmarkitem: null,
  
  run: function() {
    
    BookmarkList.show();

    
    BookmarkList.toggleManage();

    waitFor(gCurrentTest.onBookmarksReady, function() { return document.getElementById("bookmark-items").manageUI == true; });
  },
  
  onBookmarksReady: function() {
    var bookmarkitems = document.getElementById("bookmark-items");
    gCurrentTest.bookmarkitem = document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_02);
    gCurrentTest.bookmarkitem.click();

    waitFor(gCurrentTest.onEditorReady, function() { return gCurrentTest.bookmarkitem.isEditing == true; });
  },
  
  onEditorReady: function() {
    var removebutton = document.getAnonymousElementByAttribute(gCurrentTest.bookmarkitem, "anonid", "remove-button");
    removebutton.click();
    
    var bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02));
    ok(bookmark == -1, testURL_02 + " should no longer in bookmark");
    bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01));
    ok(bookmark == -1, testURL_01 + " should no longer in bookmark");

    BookmarkList.close();

    runNextTest();
  }
});



function uri(spec) {
  return ioService.newURI(spec, null, null);
}

function waitFor(callback, test, timeout) {
  if (test()) {
    callback();
    return;
  }

  timeout = timeout || Date.now();
  if (Date.now() - timeout > 1000)
    throw "waitFor timeout";
  setTimeout(waitFor, 50, callback, test, timeout);
}

