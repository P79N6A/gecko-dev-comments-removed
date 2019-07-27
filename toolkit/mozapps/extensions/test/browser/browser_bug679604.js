






var gManagerWindow;

function test() {
  waitForExplicitFinish();

  open_manager(null, function(aWindow) {
    var categories = aWindow.document.getElementById("categories");
    categories.setAttribute("last-selected", "foo");
    aWindow.document.persist("categories", "last-selected");

    close_manager(aWindow, function() {
      Services.prefs.clearUserPref(PREF_UI_LASTCATEGORY);

      open_manager(null, function(aWindow) {
        is(new CategoryUtilities(aWindow).selectedCategory, "discover",
           "Should have loaded the right view");

        close_manager(aWindow, finish);
      });
    });
  });
}
