





const PREF_DISCOVERURL = "extensions.webservice.discoverURL";

var gManagerWindow;
var gCategoryUtilities;

function test() {
  
  Services.prefs.setCharPref(PREF_DISCOVERURL, TESTROOT + "releaseNotes.xhtml");

  waitForExplicitFinish();

  run_next_test();
}

function end_test() {
  finish();
}

function getURL(browser) {
  var url = browser.currentURI.spec;
  var pos = url.indexOf("#");
  if (pos != -1)
    return url.substring(0, pos);
  return url;
}


add_test(function() {
  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);

    gCategoryUtilities.openType("discover", function() {
      var browser = gManagerWindow.document.getElementById("discover-browser");
      is(getURL(browser), TESTROOT + "releaseNotes.xhtml", "Should have loaded the right url");

      close_manager(gManagerWindow, run_next_test);
    });
  });
});



add_test(function() {
  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    is(gCategoryUtilities.selectedCategory, "discover", "Should have loaded the right view");

    var browser = gManagerWindow.document.getElementById("discover-browser");
    is(getURL(browser), TESTROOT + "releaseNotes.xhtml", "Should have loaded the right url");

    close_manager(gManagerWindow, run_next_test);
  });
});



add_test(function() {
  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    gCategoryUtilities.openType("extension", function() {
      close_manager(gManagerWindow, function() {
        open_manager("addons://discover/", function(aWindow) {
          gManagerWindow = aWindow;
          gCategoryUtilities = new CategoryUtilities(gManagerWindow);
          is(gCategoryUtilities.selectedCategory, "discover", "Should have loaded the right view");

          var browser = gManagerWindow.document.getElementById("discover-browser");
          is(getURL(browser), TESTROOT + "releaseNotes.xhtml", "Should have loaded the right url");

          close_manager(gManagerWindow, run_next_test);
        });
      });
    });
  });
});
