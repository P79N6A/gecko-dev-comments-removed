




































let newTabs = [];

function test() {
  waitForExplicitFinish();

  let tabOne = gBrowser.addTab();
  let tabTwo = gBrowser.addTab("http://mochi.test:8888/");

  let browser = gBrowser.getBrowserForTab(tabTwo);
  let onLoad = function() {
    browser.removeEventListener("load", onLoad, true);
    
    
    window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
    ok(!TabView.isVisible(), "Tab View is hidden");
    TabView.toggle();
  }
  browser.addEventListener("load", onLoad, true);
  newTabs = [ tabOne, tabTwo ];
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let search = contentWindow.document.getElementById("search");
  let searchButton = contentWindow.document.getElementById("searchbutton");

  ok(searchButton, "Search button exists");
  
  let onSearchEnabled = function() {
    ok(search.style.display != "none", "Search is enabled");
    contentWindow.removeEventListener(
      "tabviewsearchenabled", onSearchEnabled, false);
    searchTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchenabled", onSearchEnabled, false);
  
  EventUtils.sendMouseEvent({ type: "mousedown" }, searchButton, contentWindow);
}

function searchTest(contentWindow) {
  let searchBox = contentWindow.document.getElementById("searchbox");

  
  let tabNames = [];
  let tabItems = contentWindow.TabItems.getItems();

  ok(tabItems.length == 3, "Have three tab items");
  
  tabItems.forEach(function(tab) {
    tabNames.push(tab.nameEl.innerHTML);
  });
  ok(tabNames[0] && tabNames[0].length > 2, 
     "The title of tab item is longer than 2 chars")

  
  searchBox.setAttribute("value", "");
  ok(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length == 0,
     "Match nothing if it's an empty string");

  
  searchBox.setAttribute("value", tabNames[0].charAt(0));
  ok(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length == 0,
     "Match nothing if the length of search term is less than 2");

  
  searchBox.setAttribute("value", tabNames[2]);
  ok(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length == 1,
     "Match something when the whole title exists");
  
  
  searchBox.setAttribute("value", tabNames[0].substr(1));
  contentWindow.performSearch();
  ok(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length == 2,
     "Match something when a part of title exists");

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    ok(!TabView.isVisible(), "Tab View is hidden");

    gBrowser.removeTab(newTabs[0]);
    gBrowser.removeTab(newTabs[1]);

    finish();
  }
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  EventUtils.synthesizeKey("VK_ENTER", {});
}
