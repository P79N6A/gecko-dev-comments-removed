


let newTabs = [];


function test() {
  waitForExplicitFinish();

  
  let urlBase = "http://mochi.test:8888/browser/browser/components/tabview/test/";
  let tabOne = gBrowser.addTab(urlBase + "search1.html");
  let tabTwo = gBrowser.addTab(urlBase + "search2.html");
  newTabs = [ tabOne, tabTwo ];

  
  let stillToLoad = 0; 
  let onLoad = function() {
    this.removeEventListener("load", onLoad, true);
    
    stillToLoad--; 
    if (!stillToLoad) {    
      
      window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
      ok(!TabView.isVisible(), "Tab View is hidden");
      TabView.toggle();
    }
  }
  
  newTabs.forEach(function(tab) {
    stillToLoad++; 
    gBrowser.getBrowserForTab(tab).addEventListener("load", onLoad, true);
  });
}


function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let search = contentWindow.document.getElementById("search");
  let searchButton = contentWindow.document.getElementById("searchbutton");

  ok(searchButton, "Search button exists");

  let onSearchEnabled = function() {
    contentWindow.removeEventListener(
      "tabviewsearchenabled", onSearchEnabled, false);

    ok(search.style.display != "none", "Search is enabled");

    let searchBox = contentWindow.document.getElementById("searchbox");
    ok(contentWindow.document.hasFocus() && 
       contentWindow.document.activeElement == searchBox, 
       "The search box has focus");

    searchTest(contentWindow);
  }
  contentWindow.addEventListener("tabviewsearchenabled", onSearchEnabled, false);
  
  EventUtils.sendMouseEvent({ type: "mousedown" }, searchButton, contentWindow);
}


function searchTest(contentWindow) {
  let searchBox = contentWindow.document.getElementById("searchbox");

  
  let tabItems = contentWindow.TabItems.getItems();
  ok(tabItems.length == 3, "Have three tab items");
  tabItems.forEach(function(tabItem) {
    contentWindow.TabItems._update(tabItem.tab);
  });

  
  searchBox.setAttribute("value", "");
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 0,
     "Match nothing if it's an empty string");

  
  searchBox.setAttribute("value", "s");
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 0,
     "Match nothing if the length of search term is less than 2");

  
  searchBox.setAttribute("value", "search test 1");
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 1,
     "Match something when the whole title exists");
  
  
  searchBox.setAttribute("value", "search");
  contentWindow.Search.perform();
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 2,
     "Match something when a part of title exists");

  
  searchBox.setAttribute("value", "search1.html");
  contentWindow.Search.perform();
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 1,
     "Match something when a unique part of a url exists");
   
  
  searchBox.setAttribute("value", "tabview");
  contentWindow.Search.perform();
  is(new contentWindow.TabMatcher(
      searchBox.getAttribute("value")).matched().length, 2,
     "Match something when a common part of a url exists");
     
  cleanup(contentWindow);
}


function cleanup(contentWindow) {       
  contentWindow.Search.hide(null);     
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
