




 
var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService); 
var thread = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager).currentThread;
var testURL_01 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
var testURL_02 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_02.html";
var chromeWindow = window;

















  


var gTests = [];
var gCurrentTest = null;



function test() {
  
  ok(isnot, "Mochitest must be in context");
  ok(ioService, "nsIIOService must be in context");
  ok(thread, "nsIThreadManager must be in context");
  ok(PlacesUtils, "PlacesUtils in context");
  ok(EventUtils, "EventUtils in context");
  ok(chromeWindow, "ChromeWindow in context");
  
  ok(true, "*** Starting test browser_bookmark_tags.js\n");  
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
  desc: "Test adding tags to a bookmark",
  isCompleted: false,
  _currenttab: null,
  
  run: function() {
    _currenttab = chromeWindow.Browser.addTab(testURL_02, true);
    var handlepageload = function() {
      _currenttab.browser.removeEventListener("load", handlepageload, true);
      gCurrentTest.onPageLoad();
    };
    _currenttab.browser.addEventListener("load", handlepageload , true);
  },
  
  onPageLoad: function() {
    chromeWindow.BrowserUI.doCommand("cmd_star");
    chromeWindow.BrowserUI.showBookmarks();  
    chromeWindow.BookmarkList.toggleManage();
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_02);
    var editbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "edit-button");
    editbutton.click();
    
    var tagstextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, tag-three, tag4";
    
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    var tagsarray = PlacesUtils.tagging.getTagsForURI(uri(testURL_02), {});
    is(tagsarray.length, 4, "All tags are associated with specified bookmark");
    
    chromeWindow.BookmarkList.close();
    chromeWindow.Browser.closeTab(_currenttab);
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test editing tags to bookmark",  
  isCompleted: false,

  run: function() {
    chromeWindow.BrowserUI.showBookmarks();  
    chromeWindow.BookmarkList.toggleManage();
    
    var taggeduri = PlacesUtils.tagging.getURIsForTag("tag-three");
    is(taggeduri[0].spec, testURL_02, "Old tag still associated with bookmark");
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_02);
    var editbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "edit-button");
    editbutton.click();
    
    var tagstextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, edited-tag-three, tag4";
    
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    var untaggeduri = PlacesUtils.tagging.getURIsForTag("tag-three");
    is(untaggeduri, "", "Old tag is not associated with any bookmark");
    taggeduri = PlacesUtils.tagging.getURIsForTag("edited-tag-three");
    is(taggeduri[0].spec, testURL_02, "New tag is added to bookmark");
    var tagsarray = PlacesUtils.tagging.getTagsForURI(uri(testURL_02), {});
    is(tagsarray.length, 4, "Bookmark still has same number of tags");
    
    chromeWindow.BookmarkList.close();
    
    gCurrentTest.isCompleted = true;
  },

});




gTests.push({
  desc: "Test removing tags from a bookmark",
  _currenttab: null,

  run: function() {
    chromeWindow.BrowserUI.showBookmarks();  
    chromeWindow.BookmarkList.toggleManage();
    
    var bookmarkitems = chromeWindow.document.getElementById("bookmark-items");
    var bookmarkitem = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitems, "uri", testURL_02);
    var editbutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "edit-button");    
    editbutton.click();
    
    var tagstextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "tags");
    tagstextbox.value = "tagone, tag two, tag4";
    
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    var untaggeduri = PlacesUtils.tagging.getURIsForTag("edited-tag-three");
    is(untaggeduri, "", "Old tag is not associated with any bookmark");
    var tagsarray = PlacesUtils.tagging.getTagsForURI(uri(testURL_02), {});
    is(tagsarray.length, 3, "Tag is successfully deleted");
    
    PlacesUtils.bookmarks.removeItem(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02)));
    chromeWindow.BookmarkList.close();
    
    gCurrentTest.isCompleted = true;
  },
  
});



function uri(spec) {
  return ioService.newURI(spec, null, null);
}
