


const PRELOAD_PREF = "browser.newtab.preload";

function runTests() {
  
  Services.prefs.setBoolPref(PRELOAD_PREF, false);

  
  let afterLoadProvider = {
    getLinks: function(callback) {
      this.callback = callback;
    },
    addObserver: function() {},
  };
  NewTabUtils.links.addProvider(afterLoadProvider);

  
  addNewTabPageTab();
  let browser = gWindow.gBrowser.selectedTab.linkedBrowser;
  yield browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    
    
    executeSoon(() => afterLoadProvider.callback([]));
  }, true);

  let {_cellMargin, _cellHeight, _cellWidth, node} = getGrid();
  isnot(_cellMargin, null, "grid has a computed cell margin");
  isnot(_cellHeight, null, "grid has a computed cell height");
  isnot(_cellWidth, null, "grid has a computed cell width");
  let {height, maxHeight, maxWidth} = node.style;
  isnot(height, "", "grid has a computed grid height");
  isnot(maxHeight, "", "grid has a computed grid max-height");
  isnot(maxWidth, "", "grid has a computed grid max-width");

  
  NewTabUtils.links.removeProvider(afterLoadProvider);
  Services.prefs.clearUserPref(PRELOAD_PREF);
}
