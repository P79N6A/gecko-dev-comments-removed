var testURL_01 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
var testURL_02 = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_02.html";
var testURL_03 = "chrome://mochikit/content/browser/mobile/chrome/browser_english_title.html";
var testURL_04 = "chrome://mochikit/content/browser/mobile/chrome/browser_no_title.html";
var pngURL = "data:image/gif;base64,R0lGODlhCwALAIAAAAAA3pn/ZiH5BAEAAAEALAAAAAALAAsAAAIUhA+hkcuO4lmNVindo7qyrIXiGBYAOw==";


var gTests = [];
var gCurrentTest = null;

function pageLoaded(url) {
  return function() {
    let tab = gCurrentTest._currentTab;
    return !tab.isLoading() && tab.browser.currentURI.spec == url;
  }
}



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
      
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Loading a page into the URLBar with VK_RETURN",
  _currentTab: null,

  run: function() {
    gCurrentTest._currentTab = Browser.addTab(testURL_01, true);

    
    waitFor(gCurrentTest.onPageReady, pageLoaded(testURL_01));
  },

  onPageReady: function() {
    
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._currentTab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._currentTab.browser.canGoForward, "Forward button check");

    
    let urlbarEdit = document.getElementById("urlbar-edit");
    EventUtils.synthesizeMouse(urlbarEdit, urlbarEdit.width / 2, urlbarEdit.height / 2, {});

    
    window.addEventListener("popupshown", gCurrentTest.onFocusReady, false);
  },

  onFocusReady: function() {
    window.removeEventListener("popupshown", gCurrentTest.onFocusReady, false);

    
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "edit", "URL Mode is set to 'edit'");

    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._currentTab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._currentTab.browser.canGoForward, "Forward button check");

    
    let search = document.getElementById("tool-search");
    let searchStyle = window.getComputedStyle(search, null);
    is(searchStyle.visibility, "visible", "SEARCH is visible");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "collapse", "RELOAD is hidden");

    
    EventUtils.synthesizeString(testURL_02, window);
    EventUtils.synthesizeKey("VK_RETURN", {}, window)

    
    waitFor(gCurrentTest.onPageFinish, pageLoaded(testURL_02));
  },

  onPageFinish: function() {
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let search = document.getElementById("tool-search");
    let searchStyle = window.getComputedStyle(search, null);
    is(searchStyle.visibility, "collapse", "SEARCH is hidden");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "visible", "RELOAD is visible");

    let uri = gCurrentTest._currentTab.browser.currentURI.spec;
    is(uri, testURL_02, "URL Matches newly created Tab");

    
    gCurrentTest._currentTab.browser.goBack();

    
    waitFor(gCurrentTest.onPageBack, pageLoaded(testURL_01));
  },

  onPageBack: function() {
    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._currentTab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._currentTab.browser.canGoForward, "Forward button check");

    Browser.closeTab(gCurrentTest._currentTab);
    runNextTest();
  }
});




gTests.push({
  desc: "Check for text in the url bar for no title, with title and title change after pageload",
  _currentTab: null,

  run: function() {
    gCurrentTest._currentTab = Browser.addTab(testURL_03, true);

    
    messageManager.addMessageListener("pageshow", function() {
    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_03) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageReady();
    }});
  },

  onPageReady: function() {
    let urlbarEdit = document.getElementById("urlbar-edit");
    is(urlbarEdit.value, "English Title Page", "The title must be displayed in urlbar");
    Browser.closeTab(gCurrentTest._currentTab);
    gCurrentTest._currentTab = Browser.addTab(testURL_04, true);

    messageManager.addMessageListener("pageshow", function() {
    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_04) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageReady2();
    }});
  },

  onPageReady2: function(){
    let urlbarEdit = document.getElementById("urlbar-edit");
    is(urlbarEdit.value, testURL_04, "The url for no title must be displayed in urlbar");
    Browser.closeTab(gCurrentTest._currentTab);

    
    gCurrentTest._currentTab = Browser.addTab(testURL_01, true);
    messageManager.addMessageListener("pageshow", function() {
    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_01) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageReady3();
    }});
  },

  onPageReady3: function(){
    let urlbarEdit = document.getElementById("urlbar-edit");
    is(urlbarEdit.value, "Browser Blank Page 01", "The title of the first page must be displayed");
    EventUtils.synthesizeMouse(urlbarEdit, urlbarEdit.width / 2, urlbarEdit.height / 2, {});

    
    window.addEventListener("popupshown", gCurrentTest.onFocusReady, false);
  },

  onFocusReady: function() {
    window.removeEventListener("popupshown", gCurrentTest.onFocusReady, false);
    EventUtils.synthesizeString(testURL_02, window);
    EventUtils.synthesizeKey("VK_RETURN", {}, window)

    messageManager.addMessageListener("pageshow", function() {
    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_02) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageFinish();
    }});
  },

  onPageFinish: function() {
    let urlbarEdit = document.getElementById("urlbar-edit");
    is(urlbarEdit.value, "Browser Blank Page 02", "The title of the second page must be displayed");
    Browser.closeTab(gCurrentTest._currentTab);
    runNextTest();
  }
});


gTests.push({
  desc: "Check for appearance of the favicon",
  _currentTab: null,

  run: function() {
    gCurrentTest._currentTab = Browser.addTab(testURL_04, true);
    messageManager.addMessageListener("pageshow", function() {

    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_04) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageReady();
    }});
  },

  onPageReady: function() {
    let favicon = document.getElementById("urlbar-favicon");
    is(favicon.src, "", "The default favicon must be loaded");
    Browser.closeTab(gCurrentTest._currentTab);

    gCurrentTest._currentTab = Browser.addTab(testURL_03, true);
    messageManager.addMessageListener("pageshow", function() {
    if (gCurrentTest._currentTab.browser.currentURI.spec == testURL_03) {
      messageManager.removeMessageListener("pageshow", arguments.callee);
      gCurrentTest.onPageFinish();
    }});
  },

  onPageFinish: function(){
    let favicon = document.getElementById("urlbar-favicon");
    is(favicon.src, pngURL, "The page favicon must be loaded");
    Browser.closeTab(gCurrentTest._currentTab);
    runNextTest();
  }
});

