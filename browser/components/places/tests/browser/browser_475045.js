



var chromeUtils = {};
this._scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
this._scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", chromeUtils);

function test() {
  
  let toolbar = document.getElementById("PersonalToolbar");
  ok(toolbar, "PersonalToolbar should not be null");

  if (toolbar.collapsed) {
    setToolbarVisibility(toolbar, true);
    registerCleanupFunction(function() {
      setToolbarVisibility(toolbar, false);
    });
  }

  
  
  let placesItems = document.getElementById("PlacesToolbarItems");
  ok(placesItems, "PlacesToolbarItems should not be null");
  ok(placesItems.localName == "scrollbox", "PlacesToolbarItems should not be null");
  ok(placesItems.childNodes[0], "PlacesToolbarItems must have at least one child");

  







  let simulateDragDrop = function(aEffect, aMimeType) {
    const uriSpec = "http://www.mozilla.org/D1995729-A152-4e30-8329-469B01F30AA7";
    let uri = makeURI(uriSpec);
    chromeUtils.synthesizeDrop(placesItems.childNodes[0], 
                              placesItems, 
                              [[{type: aMimeType, 
                                data: uriSpec}]], 
                              aEffect, window, EventUtils);

    
    let bookmarkIds = PlacesUtils.bookmarks
                      .getBookmarkIdsForURI(uri);
    ok(bookmarkIds.length == 1, "There should be exactly one bookmark");
    
    PlacesUtils.bookmarks.removeItem(bookmarkIds[0]);

    
    ok(!PlacesUtils.bookmarks.isBookmarked(uri), "URI should be removed");
  } 
  
  
  let mimeTypes = ["text/plain", "text/unicode", "text/x-moz-url"];
  let effects = ["move", "copy", "link"];
  effects.forEach(function (effect) {
    mimeTypes.forEach(function (mimeType) {
      simulateDragDrop(effect, mimeType);
    });
  });
}
