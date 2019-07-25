





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
  desc: "Test appearance and behavior of the bookmark popup",
  _currenttab: null,
  
  run: function() {
    this._currenttab = Browser.addTab(testURL_02, true);
    function handleEvent() {
      gCurrentTest._currenttab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currenttab.browser.addEventListener("load", handleEvent , true);
  },
  
  onPageLoad: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();
    
    waitFor(gCurrentTest.onPopupReady1, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },
  
  onPopupReady1: function() {
    
    setTimeout(gCurrentTest.onPopupGone, 3000);
  },
  
  onPopupGone: function() {
    
    is(document.getElementById("bookmark-popup").hidden, true, "Bookmark popup should be auto-hidden");
    
    
    var starbutton = document.getElementById("tool-star");
    starbutton.click();
    
    waitFor(gCurrentTest.onPopupReady2, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },
  
  onPopupReady2: function() {
    
    var starbutton = document.getElementById("tool-star");
    starbutton.click();
    
    waitFor(gCurrentTest.onPopupGone2, function() { return document.getElementById("bookmark-popup").hidden == true; });
  },
  
  onPopupGone2: function() {
    
    is(document.getElementById("bookmark-popup").hidden, true, "Bookmark popup should be hidden by clicking star");
    
    
    var starbutton = document.getElementById("tool-star");
    starbutton.click();
    
    waitFor(gCurrentTest.onPopupReady3, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady3: function() {
    
    var contentarea = document.getElementById("tile-container");
    EventUtils.synthesizeMouse(contentarea, contentarea.clientWidth / 2, contentarea.clientHeight / 2, {});
    
    waitFor(gCurrentTest.onPopupGone3, function() { return document.getElementById("bookmark-popup").hidden == true; });
  },
  
  onPopupGone3: function() {
    
    is(document.getElementById("bookmark-popup").hidden, true, "Bookmark popup should be hidden by clicking in content");
    
    BrowserUI.closeTab(this._currenttab);
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Test adding tags via star icon",
  _currenttab: null,
  
  run: function() {
    this._currenttab = Browser.addTab(testURL_02, true);
    function handleEvent() {
      gCurrentTest._currenttab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currenttab.browser.addEventListener("load", handleEvent , true);
  },
  
  onPageLoad: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();

    waitFor(gCurrentTest.onPopupReady, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },
  
  onPopupReady: function() {
    var editbutton = document.getElementById("bookmark-popup-edit");
    editbutton.click();
    
    waitFor(gCurrentTest.onEditorReady, function() { return document.getElementById("bookmark-item").isEditing == true; });
  },
  
  onEditorReady: function() {
    var bookmarkitem = document.getElementById("bookmark-item");
    bookmarkitem.tags = "tagone, tag two, tag-three, tag4";
    
    var donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    waitFor(gCurrentTest.onEditorDone, function() { return document.getElementById("bookmark-container").hidden == true; });
  },

  onEditorDone: function() {
    var tagsarray = PlacesUtils.tagging.getTagsForURI(uri(testURL_02), {});
    is(tagsarray.length, 4, "All tags are added.");
    
    BrowserUI.closeTab(this._currenttab);
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Test editing uri via star icon",
  _currenttab: null,
  
  run: function() {
    this._currenttab = Browser.addTab(testURL_02, true);
    function handleEvent() {
      gCurrentTest._currenttab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currenttab.browser.addEventListener("load", handleEvent, true);
  },
  
  onPageLoad: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();    
    
    waitFor(gCurrentTest.onPopupReady, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },
  
  onPopupReady: function() {
    var editbutton = document.getElementById("bookmark-popup-edit");
    editbutton.click();
    
    waitFor(gCurrentTest.onEditorReady, function() { return document.getElementById("bookmark-item").isEditing == true; });
  },
  
  onEditorReady: function() {
    var bookmarkitem = document.getElementById("bookmark-item");    
    EventUtils.synthesizeMouse(bookmarkitem, bookmarkitem.clientWidth / 2, bookmarkitem.clientHeight / 2, {});

    var uritextbox = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "uri");
    uritextbox.value = testURL_01;

    var donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    waitFor(gCurrentTest.onEditorDone, function() { return document.getElementById("bookmark-container").hidden == true; });
  },

  onEditorDone: function() {
    isnot(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01)), -1, testURL_01 + " is now bookmarked");
    is(PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_02)), -1, testURL_02 + " is no longer bookmarked");
    
    BrowserUI.closeTab(this._currenttab);
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Test removing existing bookmark via popup",
  _currenttab: null,
  
  run: function() {
    this._currenttab = Browser.addTab(testURL_01, true);
    function handleEvent() {
      gCurrentTest._currenttab.browser.removeEventListener("load", handleEvent, true);
      gCurrentTest.onPageLoad();
    };
    this._currenttab.browser.addEventListener("load", handleEvent, true);
  },
  
  onPageLoad: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();    
    
    waitFor(gCurrentTest.onPopupReady, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },
  
  onPopupReady: function() {
    var removebutton = document.getElementById("bookmark-popup-remove");
    removebutton.click();
    
    var bookmark = PlacesUtils.getMostRecentBookmarkForURI(uri(testURL_01));
    ok(bookmark == -1, testURL_01 + " should no longer in bookmark");

    BrowserUI.closeTab(this._currenttab);

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

