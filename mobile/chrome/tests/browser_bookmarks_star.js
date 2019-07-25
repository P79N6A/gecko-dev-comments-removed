





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
  desc: "Test appearance and behavior of the bookmark popup",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_02, true);

    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageLoad();
      }
    });
  },

  onPageLoad: function() {
    let starbutton = document.getElementById("tool-star");
    starbutton.click();

    waitFor(gCurrentTest.onPopupReady1, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady1: function() {
    
    setTimeout(gCurrentTest.onPopupGone, 3000);
  },

  onPopupGone: function() {
    is(document.getElementById("bookmark-popup").hidden, false, "Bookmark popup should not be auto-hidden");
    waitFor(gCurrentTest.onPopupReady2, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady2: function() {
    
    var starbutton = document.getElementById("tool-star");
    starbutton.click();
    
    waitFor(gCurrentTest.onPopupGone2, function() { return document.getElementById("bookmark-popup").hidden == true; });
  },

  onPopupGone2: function() {
    
    is(document.getElementById("bookmark-popup").hidden, true, "Bookmark popup should be hidden by clicking star");

    
    let starbutton = document.getElementById("tool-star");
    starbutton.click();

    waitFor(gCurrentTest.onPopupReady3, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady3: function() {
    
    let contentarea = document.getElementById("browsers");
    EventUtils.synthesizeMouse(contentarea, contentarea.clientWidth / 2, contentarea.clientHeight / 2, {});

    waitFor(gCurrentTest.onPopupGone3, function() { return document.getElementById("bookmark-popup").hidden == true; });
  },

  onPopupGone3: function() {
    
    is(document.getElementById("bookmark-popup").hidden, true, "Bookmark popup should be hidden by clicking in content");
    
    BrowserUI.closeTab(this._currentTab);
    
    runNextTest();
  }
});



gTests.push({
  desc: "Test adding tags via star icon",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_02, true);

    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageLoad();
      }
    });
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
    var tagsarray = PlacesUtils.tagging.getTagsForURI(makeURI(testURL_02), {});
    is(tagsarray.length, 4, "All tags are added.");
    
    BrowserUI.closeTab(this._currentTab);
    
    runNextTest();
  }
});



gTests.push({
  desc: "Test editing uri via star icon",
  _currentTab: null,

  run: function() {
    this._currentTab = Browser.addTab(testURL_02, true);

    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageLoad();
      }
    });
  },

  onPageLoad: function() {
    let starbutton = document.getElementById("tool-star");
    starbutton.click();

    waitFor(gCurrentTest.onPopupReady, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady: function() {
    let editbutton = document.getElementById("bookmark-popup-edit");
    editbutton.click();
    
    waitFor(gCurrentTest.onEditorReady, function() { return document.getElementById("bookmark-item").isEditing == true; });
  },

  onEditorReady: function() {
    let bookmarkitem = document.getElementById("bookmark-item");    
    bookmarkitem.spec = testURL_01;

    let donebutton = document.getAnonymousElementByAttribute(bookmarkitem, "anonid", "done-button");
    donebutton.click();
    
    waitFor(gCurrentTest.onEditorDone, function() { return document.getElementById("bookmark-container").hidden == true; });
  },

  onEditorDone: function() {
    isnot(PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_01)), -1, testURL_01 + " is now bookmarked");
    is(PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_02)), -1, testURL_02 + " is no longer bookmarked");
    
    BrowserUI.closeTab(this._currentTab);
    
    runNextTest();
  }
});



gTests.push({
  desc: "Test removing existing bookmark via popup",
  _currentTab: null,
  run: function() {
    this._currentTab = Browser.addTab(testURL_01, true);

    messageManager.addMessageListener("pageshow",
    function(aMessage) {
      if (gCurrentTest._currentTab.browser.currentURI.spec != "about:blank") {
        messageManager.removeMessageListener(aMessage.name, arguments.callee);
        gCurrentTest.onPageLoad();
      }
    });
  },

  onPageLoad: function() {
    var starbutton = document.getElementById("tool-star");
    starbutton.click();

    waitFor(gCurrentTest.onPopupReady, function() { return document.getElementById("bookmark-popup").hidden == false; });
  },

  onPopupReady: function() {
    var removebutton = document.getElementById("bookmark-popup-remove");
    removebutton.click();
    
    var bookmark = PlacesUtils.getMostRecentBookmarkForURI(makeURI(testURL_01));
    ok(bookmark == -1, testURL_01 + " should no longer in bookmark");

    BrowserUI.closeTab(this._currentTab);

    runNextTest();
  }
});
