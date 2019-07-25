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
      
    }
    finally {
      
      finish();
    }
  }
}



gTests.push({
  desc: "Loading a page into the URLBar with VK_RETURN",
  _tab: null,

  run: function() {
    this._tab = Browser.addTab(testURL_01, true);

    
    waitFor(gCurrentTest.onPageReady, function() { return gCurrentTest._tab._loading == false; });
  },
  
  onPageReady: function() {
    
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._tab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._tab.browser.canGoForward, "Forward button check");

    
    let urlbarEditArea = document.getElementById("urlbar-editarea");
    EventUtils.synthesizeMouse(urlbarEditArea, urlbarEditArea.clientWidth / 2, urlbarEditArea.clientHeight / 2, {});

    
    window.addEventListener("popupshown", gCurrentTest.onFocusReady, false);
  },
  
  onFocusReady: function() {
    window.removeEventListener("popupshown", gCurrentTest.onFocusReady, false);

    
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "edit", "URL Mode is set to 'edit'");

    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._tab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._tab.browser.canGoForward, "Forward button check");

    
    let go = document.getElementById("tool-go");
    let goStyle = window.getComputedStyle(go, null);
    is(goStyle.visibility, "visible", "GO is visible");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "collapse", "RELOAD is hidden");

    
    EventUtils.synthesizeString(testURL_02, window);
    EventUtils.synthesizeKey("VK_RETURN", {}, window)

    
    waitFor(gCurrentTest.onPageFinish, function() { return urlIcons.getAttribute("mode") == "view"; });
  },

  onPageFinish: function() {
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let go = document.getElementById("tool-go");
    let goStyle = window.getComputedStyle(go, null);
    is(goStyle.visibility, "collapse", "GO is hidden");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "visible", "RELOAD is visible");

    let uri = gCurrentTest._tab.browser.currentURI.spec;
    is(uri, testURL_02, "URL Matches newly created Tab");

    
    gCurrentTest._tab.browser.goBack();

    
    waitFor(gCurrentTest.onPageBack, function() { return urlIcons.getAttribute("mode") == "view"; });
  },

  onPageBack: function() {
    
    let back = document.getElementById("tool-back");
    is(back.disabled, !gCurrentTest._tab.browser.canGoBack, "Back button check");

    
    let forward = document.getElementById("tool-forward");
    is(forward.disabled, !gCurrentTest._tab.browser.canGoForward, "Forward button check");

    Browser.closeTab(gCurrentTest._tab);
    
    runNextTest();
  }  
});



gTests.push({
  desc: "Loading a page into the URLBar with GO button",
  _tab: null,

  run: function() {
    this._tab = Browser.addTab(testURL_01, true);

    
    waitFor(gCurrentTest.onPageReady, function() { return gCurrentTest._tab._loading == false; });
  },
  
  onPageReady: function() {
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let urlbarEditArea = document.getElementById("urlbar-editarea");
    EventUtils.synthesizeMouse(urlbarEditArea, urlbarEditArea.clientWidth / 2, urlbarEditArea.clientHeight / 2, {});

    
    window.addEventListener("popupshown", gCurrentTest.onFocusReady, false);
  },
  
  onFocusReady: function() {
    window.removeEventListener("popupshown", gCurrentTest.onFocusReady, false);

    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "edit", "URL Mode is set to 'edit'");

    
    let go = document.getElementById("tool-go");
    let goStyle = window.getComputedStyle(go, null);
    is(goStyle.visibility, "visible", "GO is visible");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "collapse", "RELOAD is hidden");

    EventUtils.synthesizeString(testURL_02, window);
    EventUtils.synthesizeMouse(go, go.clientWidth / 2, go.clientHeight / 2, {});

    
    waitFor(gCurrentTest.onPageFinish, function() { return urlIcons.getAttribute("mode") == "view"; });
  },

  onPageFinish: function() {
    let urlIcons = document.getElementById("urlbar-icons");
    is(urlIcons.getAttribute("mode"), "view", "URL Mode is set to 'view'");

    
    let go = document.getElementById("tool-go");
    let goStyle = window.getComputedStyle(go, null);
    is(goStyle.visibility, "collapse", "GO is hidden");

    let stop = document.getElementById("tool-stop");
    let stopStyle = window.getComputedStyle(stop, null);
    is(stopStyle.visibility, "collapse", "STOP is hidden");

    let reload = document.getElementById("tool-reload");
    let reloadStyle = window.getComputedStyle(reload, null);
    is(reloadStyle.visibility, "visible", "RELOAD is visible");

    let uri = gCurrentTest._tab.browser.currentURI.spec;
    is(uri, testURL_02, "URL Matches newly created Tab");

    Browser.closeTab(gCurrentTest._tab);
    
    runNextTest();
  }  
});
