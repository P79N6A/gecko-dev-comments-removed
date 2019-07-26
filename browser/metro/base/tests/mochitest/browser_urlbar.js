




"use strict";

var gEdit = null;





var gEngine = null;

const kSearchEngineName = "Foo";
const kSearchEngineURI = chromeRoot + "res/testEngine.xml";




function addMockSearchDefault(aTimeoutMs) {
  let deferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let timerID = 0;

  function engineAddObserver(aSubject, aTopic, aData) {
    if (aData != "engine-added")
      return;

    gEngine = Services.search.getEngineByName(kSearchEngineName);
    Services.obs.removeObserver(engineAddObserver, "browser-search-engine-modified");
    clearTimeout(timerID);
    gEngine.hidden = false;
    ok(gEngine, "mock engine was added");
    deferred.resolve();
  }

  if (gEngine) {
    deferred.resolve();
    return deferred.promise;
  }

  timerID = setTimeout(function ids_canceller() {
    Services.obs.removeObserver(engineAddObserver, "browser-search-engine-modified");
    deferred.reject(new Error("search add timeout"));
  }, timeoutMs);

  Services.obs.addObserver(engineAddObserver, "browser-search-engine-modified", false);
  Services.search.addEngine(kSearchEngineURI, Ci.nsISearchEngine.DATA_XML,
                            "data:image/x-icon,%00", false);
  return deferred.promise;
}





function removeMockSearchDefault(aTimeoutMs) {
  let deferred = Promise.defer();
  let timeoutMs = aTimeoutMs || kDefaultWait;
  let timerID = 0;

  function engineRemoveObserver(aSubject, aTopic, aData) {
    if (aData != "engine-removed")
      return;

    clearTimeout(timerID);
    gEngine = null;
    Services.obs.removeObserver(engineRemoveObserver, "browser-search-engine-modified");
    deferred.resolve();
  }

  if (!gEngine) {
    deferred.resolve();
    return deferred.promise;
  }

  timerID = setTimeout(function ids_canceller() {
    Services.obs.removeObserver(engineRemoveObserver, "browser-search-engine-modified");
    deferred.reject(new Error("search remove timeout"));
  }, timeoutMs);

  Services.obs.addObserver(engineRemoveObserver, "browser-search-engine-modified", false);
  Services.search.removeEngine(gEngine);
  return deferred.promise;
}





function test() {
  waitForExplicitFinish();
  Task.spawn(function(){
    yield addTab("about:blank");
  }).then(runTests);
}

function setUp() {
  if (!gEdit)
    gEdit = document.getElementById("urlbar-edit");
  yield showNavBar();
}

function tearDown() {
  yield removeMockSearchDefault();
  Browser.closeTab(Browser.selectedTab, { forceClose: true });
}

gTests.push({
  desc: "search engines update",
  setUp: setUp,
  tearDown: tearDown,
  run: function testSearchEngine() {
    
    if (gEdit.popup._searches == undefined) {
      gEdit.openPopup();
      gEdit.closePopup();
    }

    let numSearches = gEdit.popup._searches.itemCount;
    function getEngineItem() {
      return gEdit.popup._searches.querySelector("richgriditem[value="+kSearchEngineName+"]");
    }

    yield addMockSearchDefault();
    ok(gEdit.popup._searches.itemCount == numSearches + 1, "added search engine count");
    ok(getEngineItem(), "added search engine item");

    yield removeMockSearchDefault();
    ok(gEdit.popup._searches.itemCount == numSearches, "normal search engine count");
    ok(!getEngineItem(), "added search engine item");
  }
});

gTests.push({
  desc: "display autocomplete while typing, handle enter",
  setUp: setUp,
  tearDown: tearDown,
  run: function testUrlbarTyping() {
    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");
    ok(!gEdit.popup.popupOpen, "focus urlbar: popup not open yet");

    EventUtils.sendString("about:blank", window);
    let opened = yield waitForCondition(() => gEdit.popup.popupOpen);
    ok(opened, "type in urlbar: popup opens");

    EventUtils.synthesizeKey("VK_RETURN", {}, window);
    let closed = yield waitForCondition(() => !gEdit.popup.popupOpen);
    ok(closed, "hit enter in urlbar: popup closes, page loads");
    ok(!gEdit.isEditing, "hit enter in urlbar: not in editing mode");
  }
});

gTests.push({
  desc: "Control-Enter in urlbar",
  setUp: setUp,
  tearDown: tearDown,
  run: function () {
    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");

    EventUtils.sendString("example", window);
    EventUtils.synthesizeKey("VK_RETURN", { accelKey: true }, window);
    is(gEdit.value, "www.example.com", "Control-enter adds www. and .com");
    ok(!gEdit.isEditing, "hit enter in urlbar: not in editing mode");
  }
});

gTests.push({
  desc: "Shift-Enter in urlbar",
  setUp: setUp,
  tearDown: tearDown,
  run: function () {
    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");

    EventUtils.sendString("example", window);
    EventUtils.synthesizeKey("VK_RETURN", { shiftKey: true }, window);
    is(gEdit.value, "www.example.net", "Shift-enter adds www. and .net");
    ok(!gEdit.isEditing, "hit enter in urlbar: not in editing mode");
  }
});

gTests.push({
  desc: "Control-Shift-Enter in urlbar",
  setUp: setUp,
  tearDown: tearDown,
  run: function () {
    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");

    EventUtils.sendString("example", window);
    EventUtils.synthesizeKey("VK_RETURN", { accelKey: true, shiftKey: true }, window);
    is(gEdit.value, "www.example.org", "Shift-enter adds www. and .org");
    ok(!gEdit.isEditing, "hit enter in urlbar: not in editing mode");
  }
});

gTests.push({
  desc: "display and select a search with keyboard",
  setUp: setUp,
  tearDown: tearDown,
  run: function testSearchKeyboard() {
    yield addMockSearchDefault();

    yield waitForCondition(() => !Browser.selectedTab.isLoading());

    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");
    ok(!gEdit.popup.popupOpen, "focus urlbar: popup not open yet");

    let search = "mozilla";
    EventUtils.sendString(search, window);
    yield waitForCondition(() => gEdit.popup.popupOpen);

    
    

    EventUtils.synthesizeKey("VK_DOWN", {}, window);
    is(gEdit.popup.selectedIndex, -1, "key select search: no result selected");
    is(gEdit.popup._searches.selectedIndex, 0, "key select search: first search selected");

    let engines = Services.search.getVisibleEngines();
    for (let i = 0, max = engines.length - 1; i < max; i++) {
      is(gEdit.popup._searches.selectedIndex, i, "key select search: next index");
      EventUtils.synthesizeKey("VK_DOWN", {}, window);
    }

    let existingValue = gEdit.value;
    EventUtils.synthesizeKey("VK_RETURN", {}, window);

    yield waitForCondition(() => gEdit.value != existingValue);

    let closed = yield waitForCondition(() => !gEdit.popup.popupOpen);
    ok(closed, "hit enter in urlbar: popup closes, page loads");
    ok(!gEdit.isEditing, "hit enter in urlbar: not in editing mode");

    let searchSubmission = gEngine.getSubmission(search, null);
    let trimmedSubmission = gEdit.trimValue(searchSubmission.uri.spec);
    is(gEdit.value, trimmedSubmission, "hit enter in urlbar: search conducted");

    yield removeMockSearchDefault();
  }
});

gTests.push({
  desc: "display and select a search with touch",
  setUp: setUp,
  tearDown: tearDown,
  run: function testUrlbarSearchesTouch() {
    yield addMockSearchDefault();

    yield waitForCondition(() => !Browser.selectedTab.isLoading());

    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");
    ok(!gEdit.popup.popupOpen, "focus urlbar: popup not open yet");

    let search = "mozilla";
    EventUtils.sendString(search, window);
    yield waitForCondition(() => gEdit.popup.popupOpen);

    sendElementTap(window, gEdit.popup._searches.lastChild);

    let closed = yield waitForCondition(() => !gEdit.popup.popupOpen);
    ok(closed, "tap search option: popup closes, page loads");
    ok(!gEdit.isEditing, "tap search option: not in editing mode");

    let searchSubmission = gEngine.getSubmission(search, null);
    let trimmedSubmission = gEdit.trimValue(searchSubmission.uri.spec);
    is(gEdit.value, trimmedSubmission, "tap search option: search conducted");
  }
});

gTests.push({
  desc: "bug 897131 - url bar update after content tap + edge swipe",
  setUp: setUp,
  tearDown: tearDown,
  run: function testUrlbarTyping() {
    let tab = yield addTab("about:mozilla");

    sendElementTap(window, gEdit);
    ok(gEdit.isEditing, "focus urlbar: in editing mode");
    ok(!gEdit.popup.popupOpen, "focus urlbar: popup not open yet");

    EventUtils.sendString("about:blank", window);
    let opened = yield waitForCondition(() => gEdit.popup.popupOpen);
    ok(opened, "type in urlbar: popup opens");

    sendElementTap(window, tab.browser);

    let closed = yield waitForCondition(() => !gEdit.popup.popupOpen);
    ok(closed, "autocomplete closed after tap on content");
    ok(!ContextUI.navbarVisible, "navbar closed");

    let event = document.createEvent("Events");
    event.initEvent("MozEdgeUICompleted", true, false);
    window.dispatchEvent(event);

    ok(ContextUI.navbarVisible, "navbar visible");
    is(gEdit.value, "about:mozilla", "url bar text refreshed");
  }
});

gTests.push({
  desc: "Bug 916383 - Invisible autocomplete items selectable by keyboard when 'your results' not shown",
  setUp: setUp,
  tearDown: tearDown,
  run: function testBug916383() {
    sendElementTap(window, gEdit);

    let bookmarkItem = Browser.selectedBrowser.contentWindow.BookmarksStartView._grid.querySelector("richgriditem");
    
    let label = bookmarkItem.getAttribute("label");

    EventUtils.sendString(label, window);

    let opened = yield waitForCondition(() => gEdit.popup.popupOpen);
    yield waitForCondition(() => gEdit.popup._results.itemCount > 0);

    ok(!gEdit.popup._resultsContainer.hidden, "'Your results' are visible");
    ok(gEdit.popup._results.itemCount > 0, "'Your results' are populated");

    
    EventUtils.sendString("zzzzzzzzzzzzzzzzzz", window);

    yield waitForCondition(() => gEdit.popup._resultsContainer.hidden);

    ok(gEdit.popup._resultsContainer.hidden, "'Your results' are hidden");
    ok(gEdit.popup._results.itemCount === 0, "'Your results' are empty");

    EventUtils.synthesizeKey("VK_DOWN", {}, window);
    is(gEdit.popup._searches.selectedIndex, 0, "key select search: first search selected");
  }
});

