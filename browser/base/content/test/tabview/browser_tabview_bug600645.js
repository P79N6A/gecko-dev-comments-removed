


let newTab;

function test() {
  waitForExplicitFinish();

  newTab = gBrowser.addTab();
  gBrowser.pinTab(newTab);

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  let contentWindow = document.getElementById("tab-view").contentWindow;
  is(contentWindow.GroupItems.groupItems.length, 1, 
     "There is one group item on startup");

  let groupItem = contentWindow.GroupItems.groupItems[0];
  let icon = contentWindow.iQ(".appTabIcon", groupItem.$appTabTray)[0];
  let $icon = contentWindow.iQ(icon);

  is($icon.data("xulTab"), newTab, 
     "The app tab icon has the right tab reference")
  
  is($icon.attr("src"), contentWindow.Utils.defaultFaviconURL, 
     "The icon is showing the default fav icon for blank tab");

  let errorHandler = function(event) {
    newTab.removeEventListener("error", errorHandler, false);

    
    
    
    executeSoon(function() {
      is($icon.attr("src"), contentWindow.Utils.defaultFaviconURL, 
         "The icon is showing the default fav icon");

      
      gBrowser.removeTab(newTab);
      let endGame = function() {
        window.removeEventListener("tabviewhidden", endGame, false);

        ok(!TabView.isVisible(), "Tab View is hidden");
        finish();
      }
      window.addEventListener("tabviewhidden", endGame, false);
      TabView.toggle();
    });
  };
  newTab.addEventListener("error", errorHandler, false);

  newTab.linkedBrowser.loadURI(
    "http://mochi.test:8888/browser/browser/base/content/test/tabview/test_bug600645.html");
}
