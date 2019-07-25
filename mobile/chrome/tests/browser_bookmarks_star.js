






















 
var thread = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager).currentThread;
var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService); 
var testURL_01 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
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
  
  ok(true, "*** Starting test browser_bookmark_star.js\n");  
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
  desc: "Test adding tags via star icon",
  isCompleted: false,
  _currenttab: null,
  
  run: function() {
    _currenttab = chromeWindow.Browser.addTab(testURL_02, true);
    var handleevent1 = function() {
      _currenttab.browser.removeEventListener("load", handleevent1, true);
      gCurrentTest.onPageLoad();
    };
    _currenttab.browser.addEventListener("load", handleevent1 , true);
  },
  
  onPageLoad: function() {
    var starbutton = chromeWindow.document.getElementById("tool-star");
    starbutton.click();
    var starbutton = chromeWindow.document.getElementById("tool-star");
    starbutton.click();
    
    var bookmarkitem = chromeWindow.document.getElementById("bookmark-item");
    
    
    while(!bookmarkitem._isEditing) {
      thread.processNextEvent(true);
    }
    
    var uritextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "uri");
    var urispec = uritextbox.value;
    
    var tagtextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "tags");
    tagtextbox.value = "tagone, tag two, tag-three, tag4";
    
    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    var tagsarray = PlacesUtils.tagging.getTagsForURI(uri(urispec), {});
    is(tagsarray.length, 4, "All tags are added.");
    
    chromeWindow.BrowserUI.closeTab(_currenttab);
    
    gCurrentTest.isCompleted = true;
  },
  
});



gTests.push({
  desc: "Test editing uri via star icon",
  isCompleted: false,
  _currenttab: null,
  
  run: function() {
    _currenttab = chromeWindow.Browser.addTab(testURL_02, true);
    var handleevent2 = function() {
      _currenttab.browser.removeEventListener("load", handleevent2, true);
      gCurrentTest.onPageLoad();
    };
    _currenttab.browser.addEventListener("load", handleevent2, true);
  },
  
  onPageLoad: function() {
    var starbutton = chromeWindow.document.getElementById("tool-star");
    starbutton.click();    
    
    var bookmarkitem = chromeWindow.document.getElementById("bookmark-item");
    
    
    while(!bookmarkitem._isEditing) {
      thread.processNextEvent(true);
    }
    
    var uritextbox = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "uri");
    uritextbox.value = testURL_01;

    var donebutton = chromeWindow.document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    isnot(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01)), -1, testURL_01 + " is now bookmarked");
    is(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02)), -1, testURL_02 + " is no longer bookmarked");
    
    PlacesUtils.bookmarks.removeItem(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01)));
    chromeWindow.BrowserUI.closeTab(_currenttab);
    
    gCurrentTest.isCompleted = true;
  },
  
});



function uri(spec) {
  return ioService.newURI(spec, null, null);
}
