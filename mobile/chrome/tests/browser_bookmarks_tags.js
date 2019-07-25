




 
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
  desc: "Test adding tags to a bookmark",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_02, true);
    function handleEvent() {
      gCurrentTest._currentTab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currentTab.browser.addEventListener("load", handleEvent , true);
  },

  onPageLoad: function() {
    let starbutton = document.getElementById("tool-star");
    starbutton.click();

    let bookmarkItem = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02));
    ok(bookmarkItem != -1, testURL_02 + " should be added.");

    
    BookmarkList.show();

    
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);
    let tagstextbox = document.getAnonymousElementByAttribute(bookmark, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, tag-three, tag4";

    let donebutton = document.getAnonymousElementByAttribute(bookmark, "anonid", "done-button");
    donebutton.click();

    let tagsarray = PlacesUtils.tagging.getTagsForURI(makeURI(testURL_02), {});
    is(tagsarray.length, 4, "All tags are associated with specified bookmark");

    BookmarkList.close();
    Browser.closeTab(this._currentTab);

    runNextTest();
  }
});



gTests.push({
  desc: "Test editing tags to bookmark",

  run: function() {
    
    BookmarkList.show();

    
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);

    let taggeduri = PlacesUtils.tagging.getURIsForTag("tag-three");
    is(taggeduri[0].spec, testURL_02, "Old tag still associated with bookmark");

    let tagstextbox = document.getAnonymousElementByAttribute(bookmark, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, edited-tag-three, tag4";

    let donebutton = document.getAnonymousElementByAttribute(bookmark, "anonid", "done-button");
    donebutton.click();

    let untaggeduri = PlacesUtils.tagging.getURIsForTag("tag-three");
    is(untaggeduri, "", "Old tag is not associated with any bookmark");
    taggeduri = PlacesUtils.tagging.getURIsForTag("edited-tag-three");
    is(taggeduri[0].spec, testURL_02, "New tag is added to bookmark");
    let tagsarray = PlacesUtils.tagging.getTagsForURI(makeURI(testURL_02), {});
    is(tagsarray.length, 4, "Bookmark still has same number of tags");

    BookmarkList.close();

    runNextTest();
  }
});




gTests.push({
  desc: "Test removing tags from a bookmark",
  _currentTab: null,

  run: function() {
    
    BookmarkList.show();

    
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);
    bookmark.startEditing();

    waitFor(gCurrentTest.onEditorReady, function() { return bookmark.isEditing == true; });
  },

  onEditorReady: function() {
    let bookmarkItems = document.getElementById("bookmark-items");
    let bookmark = document.getAnonymousElementByAttribute(bookmarkItems, "uri", testURL_02);

    let tagstextbox = document.getAnonymousElementByAttribute(bookmark, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, tag4";

    let donebutton = document.getAnonymousElementByAttribute(bookmark, "anonid", "done-button");
    donebutton.click();

    let untaggeduri = PlacesUtils.tagging.getURIsForTag("edited-tag-three");
    is(untaggeduri, "", "Old tag is not associated with any bookmark");
    let tagsarray = PlacesUtils.tagging.getTagsForURI(makeURI(testURL_02), {});
    is(tagsarray.length, 3, "Tag is successfully deleted");

    BookmarkList.close();

    runNextTest();
  }
});
